#include "D3D12Pipeline.h"
#include "D3D12Resources.h"
#include "D3D12RHI.h"
#include "D3D12Util.h"
#include "D3D12Device.h"
#include "Core/Public/Container.h"


enum class EShaderVisibility : uint32 {
	SVVertex = 0,
	SVGeometry,
	SVPixel,
	SVCompute,
	SVCount
};

static constexpr uint32 DESCRIPTOR_RANGE_TYPE_COUNT = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER + 1;

inline uint32 GetRootParameterMapIndex(EShaderVisibility shaderVisibility, EDynamicDescriptorType type) {
	return EnumCast(type) * EnumCast(EShaderVisibility::SVCount) + EnumCast(shaderVisibility);
}

inline EShaderStageFlags ToShaderStageFlag(EShaderVisibility visibility) {
	switch (visibility) {
	case EShaderVisibility::SVVertex: return EShaderStageFlags::Vertex;
	case EShaderVisibility::SVGeometry: return EShaderStageFlags::Geometry;
	case EShaderVisibility::SVPixel: return EShaderStageFlags::Pixel;
	case EShaderVisibility::SVCompute: return EShaderStageFlags::Compute;
	default: return (EShaderStageFlags)0;
	}
}

inline D3D12_SHADER_VISIBILITY ToD3D12ShaderVisibility(EShaderVisibility visibility) {
	switch (visibility) {
	case EShaderVisibility::SVVertex: return D3D12_SHADER_VISIBILITY_VERTEX;
	case EShaderVisibility::SVGeometry: return D3D12_SHADER_VISIBILITY_GEOMETRY;
	case EShaderVisibility::SVPixel: return D3D12_SHADER_VISIBILITY_PIXEL;
	case EShaderVisibility::SVCompute:
	default: return D3D12_SHADER_VISIBILITY_ALL;
	}
}

inline uint32 GetDescriptorCounterIndex(EShaderVisibility shaderVisibility, D3D12_DESCRIPTOR_RANGE_TYPE descriptorType) {
	return (uint32)descriptorType * EnumCast(EShaderVisibility::SVCount) + EnumCast(shaderVisibility);
}

inline bool CreateRootSignatureUtil(ID3D12Device* device, TConstArrayView<D3D12_ROOT_PARAMETER> rootParameters, ID3D12RootSignature** rootSignature) {
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.NumParameters = rootParameters.Size();
	rootSignatureDesc.pParameters = rootParameters.Data();
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	// serialize root signature
	TDXPtr<ID3DBlob> serializedRootSignature;
	TDXPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSignature.Address(), errorBlob.Address());
	if (FAILED(hr)) {
		LOG_WARNING("Failed to serialize root signature: %s", (const char*)errorBlob->GetBufferPointer());
		return false;
	}
	// create root signature
	device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(rootSignature));
	if (FAILED(hr)) {
		return false;
	}
	return true;
}
bool D3D12PipelineLayout::InitLayout(const TArray<RHIShaderParamSetLayout>& layouts, ID3D12Device* device) {
	m_DescriptorCounts.Reset(0);
	m_MapLayoutToHeap.Reset();
	m_RootSignature.Reset();
	m_DescriptorTables.Reset();

	TArray<D3D12_ROOT_PARAMETER> rootParameters;
	TArray<TArray<D3D12_DESCRIPTOR_RANGE>> descriptorRanges;
	// ========== convert to d3d12 root parameters ================
	{
		TStaticArray<uint32, EnumCast(EDynamicDescriptorType::Count)* EnumCast(EShaderVisibility::SVCount)> rootParameterIndexMap(DX_INVALID_INDEX); // map heap type and shader stage to root parameter index
		TStaticArray<uint32, EnumCast(EShaderVisibility::SVCount)* DESCRIPTOR_RANGE_TYPE_COUNT> descriptorCounter(0); // count of per descriptors in different shader stages
		m_MapLayoutToHeap.Reserve(layouts.Size());
		m_DescriptorTables.Reserve(rootParameterIndexMap.Size());
		for (const RHIShaderParamSetLayout& layoutBindings : layouts) {
			auto& mapLayoutToSlotArray = m_MapLayoutToHeap.EmplaceBack();
			mapLayoutToSlotArray.Reserve(layoutBindings.Size());
			// check if contains only compute stage
			bool hasComputeStage = false;
			for (const RHIShaderBinding& binding : layoutBindings) {
				// check "compute only"
				if (!hasComputeStage) {
					hasComputeStage = EnumHasAnyFlags(binding.StageFlags, EShaderStageFlags::Compute);
				}
				if (hasComputeStage && binding.StageFlags != EShaderStageFlags::Compute) {
					LOG_ERROR("[RootParametersBuilder::Build] Shader bindings can not contain both Compute and other stages!");
					return false;
				}

				// find heap
				const EDynamicDescriptorType descriptorType = ToDynamicDescriptorType(binding.Type);
				const uint32 heapIndex = EnumCast(descriptorType);
				const uint32 slotIndex = m_DescriptorCounts[heapIndex];
				mapLayoutToSlotArray.PushBack({ descriptorType, slotIndex });

				// handle RootParameter and ranges of every shader stages
				for (uint32 i = 0; i < EnumCast(EShaderVisibility::SVCount); ++i) {
					if (!EnumHasAnyFlags(binding.StageFlags, ToShaderStageFlag((EShaderVisibility)i))) {
						continue;
					}
					const uint32 mapIndex = GetRootParameterMapIndex((EShaderVisibility)i, descriptorType);
					if (DX_INVALID_INDEX == rootParameterIndexMap[mapIndex]) {
						// create new root parameter
						rootParameterIndexMap[mapIndex] = descriptorRanges.Size();
						m_DescriptorTables.PushBack(descriptorType);
						descriptorRanges.EmplaceBack();
						auto& rootParam = rootParameters.EmplaceBack();
						rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
						rootParam.ShaderVisibility = ToD3D12ShaderVisibility((EShaderVisibility)i);
					}
					const uint32 rootParamIndex = rootParameterIndexMap[mapIndex];
					D3D12_DESCRIPTOR_RANGE& range = descriptorRanges[rootParamIndex].EmplaceBack();
					range.RangeType = ToD3D12DescriptorRangeType(binding.Type);
					range.NumDescriptors = binding.Count;
					const uint32 counterIndex = GetDescriptorCounterIndex((EShaderVisibility)i, range.RangeType);
					range.BaseShaderRegister = descriptorCounter[counterIndex]++;
					range.RegisterSpace = 0;
					range.OffsetInDescriptorsFromTableStart = slotIndex;
				}

				// increase descriptor count in heap
				m_DescriptorCounts[heapIndex] += binding.Count;
			}
		}
		for (uint32 i = 0; i < descriptorRanges.Size(); ++i) {
			auto& ranges = descriptorRanges[i];
			auto& parameter = rootParameters[i];
			parameter.DescriptorTable.NumDescriptorRanges = ranges.Size();
			parameter.DescriptorTable.pDescriptorRanges = ranges.Data();
		}
	}
	return CreateRootSignatureUtil(device, rootParameters, m_RootSignature.Address());
}

D3D12PipelineLayout::DescriptorSlot D3D12PipelineLayout::GetDescriptorSlot(uint32 set, uint32 binding) const {
	CHECK(set < m_MapLayoutToHeap.Size());
	CHECK(binding < m_MapLayoutToHeap[set].Size());
	return m_MapLayoutToHeap[set][binding];
}

D3D12GraphicsPipelineState::D3D12GraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, D3D12Device* device) : RHIGraphicsPipelineState(desc){
	// root signature
	CHECK(m_Layout.InitLayout(desc.Layout, device->GetDevice()));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12Desc{};
	d3d12Desc.pRootSignature = m_Layout.GetRootSignature();
	// shader
	if(desc.VertexShader) {
		auto bytes = ((D3D12Shader*)desc.VertexShader)->GetBytes();
		d3d12Desc.VS = { bytes.Data(), bytes.Size() };
	}
	if(desc.PixelShader) {
		auto bytes = ((D3D12Shader*)desc.PixelShader)->GetBytes();
		d3d12Desc.PS = { bytes.Data(), bytes.Size() };
	}

	// vertex input
	const RHIVertexInputInfo& vertexInput = desc.VertexInput;
	TArray<D3D12_INPUT_ELEMENT_DESC> inputElements; inputElements.Reserve(vertexInput.Attributes.Size());
	for(const RHIVertexInputInfo::AttributeDesc& attr: vertexInput.Attributes) {
		const RHIVertexInputInfo::BindingDesc& binding = vertexInput.Bindings[attr.Binding];
		D3D12_INPUT_ELEMENT_DESC& inputEle = inputElements.EmplaceBack();
		inputEle.SemanticName = attr.SemanticName;
		inputEle.SemanticIndex = attr.SemanticIndex;
		inputEle.Format = ToD3D12Format(attr.Format);
		inputEle.InputSlot = attr.Binding;
		inputEle.AlignedByteOffset = attr.Offset;
		inputEle.InputSlotClass = binding.PerInstance ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		inputEle.InstanceDataStepRate = 0; // TODO
	}
	d3d12Desc.InputLayout = { inputElements.Data(), inputElements.Size() };

	// blend state
	const RHIBlendDesc& blendDesc = desc.BlendDesc;
	D3D12_BLEND_DESC& d3d12BlendDesc = d3d12Desc.BlendState;
	d3d12BlendDesc.AlphaToCoverageEnable = false;
	d3d12BlendDesc.IndependentBlendEnable = false;
	D3D12_LOGIC_OP logicOp = blendDesc.LogicOpEnable ? D3D12_LOGIC_OP_NOOP : ToD3D12LogicOp(blendDesc.LogicOp);
	for(uint32 i=0; i<blendDesc.BlendStates.Size(); ++i) {
		const RHIBlendState& blendState = blendDesc.BlendStates[i];
		D3D12_RENDER_TARGET_BLEND_DESC& d3d12BlendState = d3d12BlendDesc.RenderTarget[i];
		d3d12BlendState.BlendEnable = blendState.Enable;
		d3d12BlendState.SrcBlend = ToD3D12Blend(blendState.ColorSrc);
		d3d12BlendState.DestBlend = ToD3D12Blend(blendState.ColorDst);
		d3d12BlendState.BlendOp = ToD3D12BlendOp(blendState.ColorBlendOp);
		d3d12BlendState.SrcBlendAlpha = ToD3D12Blend(blendState.AlphaSrc);
		d3d12BlendState.DestBlendAlpha = ToD3D12Blend(blendState.AlphaDst);
		d3d12BlendState.BlendOpAlpha = ToD3D12BlendOp(blendState.AlphaBlendOp);
		d3d12BlendState.RenderTargetWriteMask = ToD3D12ComponentFlags(blendState.ColorWriteMask);
		d3d12BlendState.LogicOpEnable = blendDesc.LogicOpEnable;
		d3d12BlendState.LogicOp = logicOp;
	}

	// rasterizer
	const RHIRasterizerState& rasterizer = desc.RasterizerState;
	D3D12_RASTERIZER_DESC& d3d12Rasterizer = d3d12Desc.RasterizerState;
	d3d12Rasterizer.FillMode = ToD3D12FillMode(rasterizer.FillMode);
	d3d12Rasterizer.CullMode = ToD3D12CullMode(rasterizer.CullMode);
	d3d12Rasterizer.FrontCounterClockwise = !rasterizer.FrontClockwise;
	d3d12Rasterizer.DepthBias = (INT)rasterizer.DepthBiasConstant;
	d3d12Rasterizer.DepthBiasClamp = rasterizer.DepthBiasClamp;
	d3d12Rasterizer.SlopeScaledDepthBias = rasterizer.DepthBiasSlope;
	d3d12Rasterizer.MultisampleEnable = desc.NumSamples > 1;
	d3d12Rasterizer.AntialiasedLineEnable = false;
	d3d12Rasterizer.ForcedSampleCount = 0;

	// depth stencil
	const RHIDepthStencilState& depthStencil = desc.DepthStencilState;
	D3D12_DEPTH_STENCIL_DESC& d3d12DepthStencil = d3d12Desc.DepthStencilState;
	d3d12DepthStencil.DepthEnable = depthStencil.DepthTest;
	d3d12DepthStencil.DepthWriteMask = depthStencil.DepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	d3d12DepthStencil.DepthFunc = ToD3D12Comparison(depthStencil.DepthCompare);
	d3d12DepthStencil.StencilEnable = depthStencil.StencilTest;
	d3d12DepthStencil.FrontFace = ToD3D12DepthStencilOpDesc(depthStencil.FrontStencil);
	d3d12DepthStencil.BackFace = ToD3D12DepthStencilOpDesc(depthStencil.BackStencil);

	d3d12Desc.PrimitiveTopologyType = ToD3D12PrimitiveTopologyType(desc.PrimitiveTopology);

	d3d12Desc.NumRenderTargets = desc.ColorFormats.Size();
	for(uint32 i=0; i<desc.ColorFormats.Size(); ++i) {
		d3d12Desc.RTVFormats[i] = ToD3D12Format(desc.ColorFormats[i]);
	}
	d3d12Desc.DSVFormat = ToD3D12Format(desc.DepthStencilFormat);

	d3d12Desc.SampleDesc.Count = desc.NumSamples;
	d3d12Desc.SampleDesc.Quality = 0;
	d3d12Desc.SampleMask = 0xffffffff;
	device->GetDevice()->CreateGraphicsPipelineState(&d3d12Desc, IID_PPV_ARGS(m_Pipeline.Address()));
	m_PrimitiveTopology = ToD3D12PrimitiveTopology(desc.PrimitiveTopology);
}

D3D12ComputePipelineState::D3D12ComputePipelineState(const RHIComputePipelineStateDesc& desc, D3D12Device* device): RHIComputePipelineState(desc) {
	CHECK(m_Layout.InitLayout(desc.Layout, device->GetDevice()));

	D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12Desc;
	d3d12Desc.pRootSignature = m_Layout.GetRootSignature();
	auto bytes = ((D3D12Shader*)desc.Shader)->GetBytes();
	d3d12Desc.CS = { bytes.Data(), bytes.Size() };
	device->GetDevice()->CreateComputePipelineState(&d3d12Desc, IID_PPV_ARGS(m_Pipeline.Address()));
}

D3D12PipelineDescriptorCache::D3D12PipelineDescriptorCache(D3D12Device* device, D3D12GraphicsPipelineState* pso): m_Device(device) {
	m_PipelineType = EPipelineType::Graphics;
	m_GraphicsPipelineState = pso;
	ReserveDescriptors();
}

D3D12PipelineDescriptorCache::D3D12PipelineDescriptorCache(D3D12Device* device, D3D12ComputePipelineState* pipeline) : m_Device(device){
	m_PipelineType = EPipelineType::Compute;
	m_ComputePipelineState = pipeline;
	ReserveDescriptors();
}

D3D12PipelineDescriptorCache::D3D12PipelineDescriptorCache(D3D12PipelineDescriptorCache&& rhs) noexcept {
	for (uint32 i = 0; i < EnumCast(EDynamicDescriptorType::Count); ++i) {
		m_DescriptorCaches[i].DynamicDescriptor = rhs.m_DescriptorCaches[i].DynamicDescriptor;
		m_DescriptorCaches[i].StaticDescriptors.Swap(rhs.m_DescriptorCaches[i].StaticDescriptors);
		m_DirtyDescriptorTables[i] = rhs.m_DirtyDescriptorTables[i];
	}
	m_Device = rhs.m_Device;
	m_PipelineData = rhs.m_PipelineData;
	m_PipelineType = rhs.m_PipelineType;
}

void D3D12PipelineDescriptorCache::SetShaderParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& param) {
	// copy descriptors
	D3D12PipelineLayout::DescriptorSlot slot = GetLayout()->GetDescriptorSlot(setIndex, bindIndex);
	DescriptorCache& cache = m_DescriptorCaches[EnumCast(slot.HeapType)];
	CHECK(slot.SlotIndex < cache.StaticDescriptors.Size());
	D3D12_CPU_DESCRIPTOR_HANDLE srcHandle{ InvalidCPUDescriptor() };
	switch (param.Type) {
	case EBindingType::UniformBuffer: {
		D3D12BufferImpl* buffer = (D3D12BufferImpl*)param.Data.Buffer;
		srcHandle = buffer->GetCBV();

	}break;
	case EBindingType::StorageBuffer: {
		D3D12BufferImpl* buffer = (D3D12BufferImpl*)param.Data.Buffer;
		srcHandle = buffer->GetUAV();
	}break;
	case EBindingType::Texture: {
		D3D12Texture* texture = (D3D12Texture*)param.Data.Texture;
		srcHandle = texture->GetDescriptor(ETexDescriptorType::SRV, param.Data.SubRes);
	}break;
	case EBindingType::StorageTexture: {
		D3D12Texture* texture = (D3D12Texture*)param.Data.Texture;
		srcHandle = texture->GetDescriptor(ETexDescriptorType::UAV, param.Data.SubRes);
	}break;
	case EBindingType::Sampler: {
		D3D12Sampler* sampler = (D3D12Sampler*)param.Data.Sampler;
		srcHandle = sampler->GetCPUHandle();
	}break;
	}
	if (srcHandle.ptr) {
		m_DirtyDescriptorTables[EnumCast(slot.HeapType)] = true;
		cache.StaticDescriptors[slot.SlotIndex] = srcHandle;
	}
}

D3D12GraphicsPipelineState* D3D12PipelineDescriptorCache::GetGraphicsPipelineState() {
	return m_PipelineType == EPipelineType::Graphics ? m_GraphicsPipelineState : nullptr;
}

D3D12ComputePipelineState* D3D12PipelineDescriptorCache::GetComputePipelineState() {
	return m_PipelineType == EPipelineType::Compute ? m_ComputePipelineState : nullptr;
}

void D3D12PipelineDescriptorCache::PreDraw(ID3D12GraphicsCommandList* cmd) {
	// set descriptor heap
	TArray<ID3D12DescriptorHeap*> heaps;
	for(uint32 i=0; i<EnumCast(EDynamicDescriptorType::Count); ++i) {
		DynamicDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetDynamicAllocator((EDynamicDescriptorType)i);
		DescriptorCache& cache = m_DescriptorCaches[i];
		if(m_DirtyDescriptorTables[i]) {
			// copy descriptors
			uint32 descriptorCount = cache.StaticDescriptors.Size();
			const DynamicDescriptorHandle dynamicDescriptor = allocator->AllocateSlot(descriptorCount);
			const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = allocator->GetCPUHandle(dynamicDescriptor);
			TArray<uint32> sizes(descriptorCount, 1);
			m_Device->GetDevice()->CopyDescriptors(1, &cpuHandle, &descriptorCount,
				descriptorCount, cache.StaticDescriptors.Data(), sizes.Data(),
				ToStaticDescriptorType((EDynamicDescriptorType)i));
			m_DirtyDescriptorTables[i] = false;
			cache.DynamicDescriptor = dynamicDescriptor;
		}
		if(cache.DynamicDescriptor.IsValid()) {
			heaps.PushBack(allocator->GetHeap(cache.DynamicDescriptor.HeapIndex));
		}
	}
	cmd->SetDescriptorHeaps(heaps.Size(), heaps.Data());

	// set root descriptor table
	auto& descriptorTables = GetLayout()->GetDescriptorTables();
	for(uint32 i=0; i<descriptorTables.Size(); ++i) {
		EDynamicDescriptorType heapType = descriptorTables[i];
		DynamicDescriptorHandle descriptor = m_DescriptorCaches[EnumCast(heapType)].DynamicDescriptor;
		if(descriptor.IsValid()) {
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_Device->GetDescriptorMgr()->GetDynamicAllocator(heapType)->GetGPUHandle(descriptor);
			if (EPipelineType::Graphics == m_PipelineType) {
				cmd->SetGraphicsRootDescriptorTable(i, gpuHandle);
			}
			else {
				cmd->SetComputeRootDescriptorTable(i, gpuHandle);
			}
		}
	}
}

const D3D12PipelineLayout* D3D12PipelineDescriptorCache::GetLayout() {
	return m_PipelineType == EPipelineType::Graphics ? &m_GraphicsPipelineState->GetLayout() : &m_ComputePipelineState->GetLayout();
}

void D3D12PipelineDescriptorCache::ReserveDescriptors() {
	for(uint32 i=0; i<EnumCast(EDynamicDescriptorType::Count); ++i) {
		DescriptorCache& cache = m_DescriptorCaches[i];
		cache.DynamicDescriptor = DynamicDescriptorHandle::InvalidHandle();
		cache.StaticDescriptors.Reset();
		if(uint32 descriptorCount = GetLayout()->GetDescriptorCount((EDynamicDescriptorType)i)) {
			cache.StaticDescriptors.Resize(descriptorCount, InvalidCPUDescriptor());
		}
		m_DirtyDescriptorTables[i] = false;
	}
}
 