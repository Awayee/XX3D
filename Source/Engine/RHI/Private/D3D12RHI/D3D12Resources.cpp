#include "D3D12Resources.h"
#include "D3D12command.h"
#include "D3D12RHI.h"
#include "D3D12Util.h"
#include "D3D12Device.h"
#include "Math/Public/Math.h"

inline ETextureFlags DescriptorTypeToFlags(ETexDescriptorType type) {
	switch (type) {
	case ETexDescriptorType::SRV: return ETextureFlags::SRV;
	case ETexDescriptorType::UAV: return ETextureFlags::UAV;
	case ETexDescriptorType::RTV: return ETextureFlags::ColorTarget;
	case ETexDescriptorType::DSV: return ETextureFlags::DepthStencilTarget;
	default: return (ETextureFlags)0;
	}
}

inline D3D12_DESCRIPTOR_HEAP_TYPE DescriptorTypeToHeapType(ETexDescriptorType type) {
	switch (type) {
	case ETexDescriptorType::SRV:
	case ETexDescriptorType::UAV: return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	case ETexDescriptorType::RTV: return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	case ETexDescriptorType::DSV: return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	default: return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	}
}

D3D12Buffer::D3D12Buffer(const RHIBufferDesc& desc, ID3D12Device* device) : RHIBuffer(desc){
	EBufferFlags flags = desc.Flags;
	uint32 bufferSize = desc.ByteSize;
	if (EnumHasAnyFlags(flags, EBufferFlags::Uniform)) {
		bufferSize = AlignConstantBufferSize(bufferSize);
	}
	auto d3d12Desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	CD3DX12_HEAP_PROPERTIES heapProperties(ToD3D12HeapTypeBuffer(flags));
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
	DX_CHECK(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &d3d12Desc, initialState, nullptr, IID_PPV_ARGS(m_Resource.Address())));
}

void D3D12Buffer::UpdateData(const void* data, uint32 byteSize, uint32 offset) {
	void* mappedData;
	DX_CHECK(m_Resource->Map(0, nullptr, &mappedData));
	memcpy(mappedData, data, byteSize);
	m_Resource->Unmap(0, nullptr);
}

void D3D12Buffer::UpdateData(Func<void(void*)>&& f) {
	void* mappedData;
	DX_CHECK(m_Resource->Map(0, nullptr, &mappedData));
	f(mappedData);
	m_Resource->Unmap(0, nullptr);
}

void D3D12Buffer::SetName(const char* name) {
	XWString nameW = String2WString(name);
	m_Resource->SetName(nameW.c_str());
}

D3D12BufferImpl::D3D12BufferImpl(const RHIBufferDesc& desc, D3D12Device* device): D3D12Buffer(desc, device->GetDevice()), m_Device(device) {
	EBufferFlags flags = desc.Flags;
	// create descriptor
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if(EnumHasAnyFlags(flags, EBufferFlags::Uniform)) {
		m_CBV = allocator->AllocateDescriptorSlot();
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = allocator->GetCPUHandle(m_CBV);
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = m_Resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = AlignConstantBufferSize(m_Desc.ByteSize);
		m_Device->GetDevice()->CreateConstantBufferView(&cbvDesc, cpuHandle);
	}
	if(EnumHasAnyFlags(flags, EBufferFlags::Storage)) {
		m_UAV = allocator->AllocateDescriptorSlot();
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = allocator->GetCPUHandle(m_UAV);
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.StructureByteStride = Math::Max<uint32>(1, GetDesc().Stride);
		uavDesc.Buffer.NumElements = GetDesc().ByteSize / uavDesc.Buffer.StructureByteStride;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		m_Device->GetDevice()->CreateUnorderedAccessView(m_Resource, nullptr, &uavDesc, cpuHandle);
	}
}

D3D12BufferImpl::~D3D12BufferImpl() {
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	allocator->FreeDescriptorSlot(m_CBV);
	allocator->FreeDescriptorSlot(m_UAV);
}

void D3D12BufferImpl::UpdateData(const void* data, uint32 byteSize, uint32 offset) {
	EBufferFlags flags = m_Desc.Flags;
	if(EnumHasAnyFlags(flags, EBufferFlags::Uniform)) {
		D3D12Buffer::UpdateData(data, byteSize, offset);
	}
	else {
		CHECK(EnumHasAnyFlags(flags, EBufferFlags::CopyDst));
		D3D12Buffer* stagingBuffer = m_Device->GetUploader()->AllocateStaging(byteSize);
		stagingBuffer->UpdateData(data, byteSize, 0);
		D3D12CommandList* uploadCmd = m_Device->GetCommandMgr()->GetQueue(EQueueType::Graphics)->GetUploadCommand();
		uploadCmd->TransitionResourceState(m_Resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST, 0);
		uploadCmd->CopyBufferToBuffer(stagingBuffer, this, 0, offset, byteSize);
		uploadCmd->TransitionResourceState(m_Resource, D3D12_RESOURCE_STATE_COPY_DEST, ToD3D12DestResourceState(flags), 0);
	}
}

D3D12_VERTEX_BUFFER_VIEW D3D12BufferImpl::GetVertexBufferView() {
	CHECK(EnumHasAnyFlags(GetDesc().Flags, EBufferFlags::Vertex));
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = m_Resource->GetGPUVirtualAddress();
	vbv.SizeInBytes = m_Desc.ByteSize;
	vbv.StrideInBytes = m_Desc.Stride;
	return vbv;
}

D3D12_INDEX_BUFFER_VIEW D3D12BufferImpl::GetIndexBufferView() {
	CHECK(EnumHasAnyFlags(GetDesc().Flags, EBufferFlags::Index));
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = m_Resource->GetGPUVirtualAddress();
	ibv.SizeInBytes = m_Desc.ByteSize;
	ibv.Format = ToD3D12Format(m_Desc.IndexFormat);
	return ibv;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12BufferImpl::GetCBV() {
	CHECK(EnumHasAnyFlags(GetDesc().Flags, EBufferFlags::Uniform));
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return allocator->GetCPUHandle(m_CBV);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12BufferImpl::GetUAV() {
	CHECK(EnumHasAnyFlags(GetDesc().Flags, EBufferFlags::Storage));
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return allocator->GetCPUHandle(m_UAV);
}

void ResourceState::Initialize(uint32 subResCount) {
	m_SubResStates.Resize(subResCount, D3D12_RESOURCE_STATE_COMMON);
	SetState(D3D12_RESOURCE_STATE_COMMON);
}

void ResourceState::SetState(D3D12_RESOURCE_STATES state) {
	m_CoverSubRes = true;
	m_ResState = (uint32)state;
}

void ResourceState::SetSubResState(uint32 subResIndex, D3D12_RESOURCE_STATES state) {
	if(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES == subResIndex || m_SubResStates.Size() == 1) {
		SetState(state);
	}
	if(m_CoverSubRes) {
		for(uint32 i=0; i<m_SubResStates.Size(); ++i) {
			m_SubResStates[i] = (D3D12_RESOURCE_STATES)m_ResState;
		}
		m_CoverSubRes = false;
	}
	CHECK(subResIndex < m_SubResStates.Size());
	m_SubResStates[subResIndex] = state;
}

D3D12_RESOURCE_STATES ResourceState::GetSubResState(uint32 subResIndex) const {
	if (m_CoverSubRes || D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES == subResIndex) {
		return (D3D12_RESOURCE_STATES)m_ResState;
	}
	CHECK(subResIndex < m_SubResStates.Size());
	return m_SubResStates[subResIndex];
}

D3D12Texture::D3D12Texture(const RHITextureDesc& desc): RHITexture(desc) {
	const uint32 subResCount = desc.MipSize * desc.ArraySize;
	m_ResState.Initialize(subResCount);
}

D3D12TextureImpl::D3D12TextureImpl(const RHITextureDesc& desc, D3D12Device* device) : D3D12Texture(desc), m_Device(device) {
	ETextureFlags flags = desc.Flags;
	D3D12_RESOURCE_DESC d3d12Desc{};
	d3d12Desc.Dimension = ToD3D12Dimension(desc.Dimension);
	d3d12Desc.Width = desc.Width;
	d3d12Desc.Height = desc.Height;
	if (desc.Dimension == ETextureDimension::Tex3D) {
		d3d12Desc.DepthOrArraySize = (uint16)desc.Depth;
	}
	else {
		d3d12Desc.DepthOrArraySize = desc.ArraySize * (uint16)GetTextureDimension2DSize(desc.Dimension);
	}
	d3d12Desc.MipLevels = desc.MipSize;
	d3d12Desc.Format = ToD3D12Format(desc.Format);
	d3d12Desc.SampleDesc.Count = desc.Samples;
	d3d12Desc.SampleDesc.Quality = 0;
	d3d12Desc.Flags = ToD3D12ResourceFlags(flags);
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	// handle clear value
	D3D12_CLEAR_VALUE clearValue;
	const D3D12_CLEAR_VALUE* pClearValue = nullptr;
	if (EnumHasAnyFlags(flags, ETextureFlags::ColorTarget | ETextureFlags::DepthStencilTarget)) {
		const auto& srcClear = desc.ClearValue;
		clearValue.Format = d3d12Desc.Format;
		memcpy(clearValue.Color, &srcClear.Color, sizeof(srcClear.Color));
		pClearValue = &clearValue;
	}
	device->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &d3d12Desc, initialState, pClearValue, IID_PPV_ARGS(m_Resource.Address()));
	m_ResState.SetState(initialState);
	uint32 arraySize = desc.Get2DArraySize();
	// reserve descriptor array
	for (uint32 i = 0; i < EnumCast(ETexDescriptorType::Count); ++i) {
		ETextureFlags needFlags = DescriptorTypeToFlags((ETexDescriptorType)i);
		if(EnumHasAnyFlags(flags, needFlags)) {
			m_Descriptors[i].Resize(arraySize);
		}
	}
}

D3D12TextureImpl::~D3D12TextureImpl() {
	// clear descriptors
	D3D12DescriptorMgr* descriptorMgr = m_Device->GetDescriptorMgr();
	for(uint32 i=0; i<m_Descriptors.Size(); ++i) {
		StaticDescriptorAllocator* allocator = descriptorMgr->GetStaticAllocator(DescriptorTypeToHeapType((ETexDescriptorType)i));
		for (auto& sds : m_Descriptors[i]) {
			for (auto& sd : sds) {
				allocator->FreeDescriptorSlot(sd.Descriptor);
			}
		}
		m_Descriptors[i].Reset();
	}
}


EResourceState GetTargetState(ETextureFlags flags) {
	if(EnumHasAnyFlags(flags, ETextureFlags::SRV)) {
		return EResourceState::ShaderResourceView;
	}
	if(EnumHasAnyFlags(flags, ETextureFlags::UAV)) {
		return EResourceState::UnorderedAccessView;
	}
	if(EnumHasAnyFlags(flags, ETextureFlags::ColorTarget)) {
		return EResourceState::ColorTarget;
	}
	if(EnumHasAnyFlags(flags, ETextureFlags::DepthStencilTarget)) {
		return EResourceState::DepthStencil;
	}
	return EResourceState::Unknown;
}

void D3D12TextureImpl::UpdateData(const void* data, uint32 byteSize, RHITextureSubRes subRes, IOffset3D offset) {
	const auto& desc = GetDesc();
	CHECK(desc.Flags & ETextureFlags::CopyDst);

	// calc buffer size, and allocate staging buffer
	const uint8 mip = subRes.MipIndex;
	const uint32 mipWidth = desc.GetMipLevelWidth(mip), mipHeight = desc.GetMipLevelHeight(mip);
	const uint32 srcRowByteSize = mipWidth * desc.GetPixelByteSize();
	const uint32 dstRowByteSize = AlignTexturePitchSize(srcRowByteSize);
	const uint32 srcSliceByteSize = mipHeight * dstRowByteSize;
	const uint32 dstSliceByteSize = AlignTextureSliceSize(srcSliceByteSize);
	const uint32 bufferSize = dstSliceByteSize * subRes.ArraySize * GetTextureDimension2DSize(subRes.Dimension);
	D3D12Buffer* stagingBuffer = m_Device->GetUploader()->AllocateStaging(bufferSize);
	// if data size is aligned, copy directly, otherwise copy row-by-row.
	if (srcRowByteSize == dstRowByteSize && srcSliceByteSize == dstSliceByteSize) {
		stagingBuffer->UpdateData(data, byteSize, 0);
	}
	else {
		stagingBuffer->UpdateData([=](void* mappedData) {
			for(uint16 arr=0; arr<subRes.ArraySize; ++arr) {
				uint8* dstData = (uint8*)mappedData + (size_t)((subRes.ArrayIndex + arr) * dstSliceByteSize);
				const uint8* srcData = (const uint8*)data + (size_t)(arr * srcSliceByteSize);
				for (uint32 row=0; row<mipHeight; ++row) {
					memcpy(dstData, srcData, (size_t)srcRowByteSize);
					dstData += (size_t)dstRowByteSize;
					srcData += (size_t)srcRowByteSize;
				}
			}
		});
	}
	// transition state
	D3D12CommandList* uploadCmd = m_Device->GetCommandMgr()->GetQueue(EQueueType::Graphics)->GetUploadCommand();
	uploadCmd->TransitionTextureState(this, EResourceState::Unknown, EResourceState::TransferDst, subRes);
	// copy buffer
	uploadCmd->CopyBufferToTexture(stagingBuffer, this, subRes, offset);
	// transition state
	uploadCmd->TransitionTextureState(this, EResourceState::TransferDst, GetTargetState(desc.Flags), subRes);
}

void D3D12TextureImpl::SetName(const char* name) {
	XWString nameW = String2WString(name);
	m_Resource->SetName(nameW.c_str());
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12TextureImpl::GetDescriptor(ETexDescriptorType type, RHITextureSubRes subRes) {
	GetDesc().LimitSubRes(subRes);
	ASSERT(IsDimensionCompatible(GetDesc().Dimension, subRes.Dimension), "[D3D12Texture::GetDescriptor] Dimension is not compatible!");
	ASSERT(EnumHasAnyFlags(GetDesc().Flags, DescriptorTypeToFlags(type)), "[D3D12Texture::GetDescriptor] Flag is not compatible!");

	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(DescriptorTypeToHeapType(type));
	auto& sds = m_Descriptors[EnumCast(type)][subRes.ArrayIndex];
	for(auto& sd: sds) {
		if(sd.SubRes == subRes) {
			return allocator->GetCPUHandle(sd.Descriptor);
		}
	}
	// allocation
	StaticDescriptorHandle descriptor = allocator->AllocateDescriptorSlot();
	D3D12_CPU_DESCRIPTOR_HANDLE handle = allocator->GetCPUHandle(descriptor);
	switch(type) {
	case ETexDescriptorType::SRV:
		CreateSRV(subRes, handle);
		break;
	case ETexDescriptorType::UAV:
		CreateUAV(subRes, handle);
		break;
	case ETexDescriptorType::RTV:
		CreateRTV(subRes, handle);
		break;
	case ETexDescriptorType::DSV:
		CreateDSV(subRes, handle);
		break;
	default:
		break;
	}
	sds.PushBack({ subRes, descriptor });
	return handle;
}

void D3D12TextureImpl::CreateSRV(RHITextureSubRes subRes, D3D12_CPU_DESCRIPTOR_HANDLE descriptor) {
	CHECK(EnumHasAnyFlags(GetDesc().Flags, ETextureFlags::SRV));
	D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = ToD3D12ViewFormat(GetDesc().Format, subRes.ViewFlags);
	const ETextureDimension texDimension = GetDesc().Dimension;
	const ETextureDimension subDimension = subRes.Dimension;
	const uint16 subPerArraySize = GetTextureDimension2DSize(subDimension);
	if(subDimension == ETextureDimension::Tex2D) {
		if(texDimension == ETextureDimension::Tex2D) {
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MostDetailedMip = subRes.MipIndex;
			desc.Texture2D.MipLevels = subRes.MipSize;
		}
		else {
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			desc.Texture2DArray.FirstArraySlice = subRes.ArrayIndex;
			desc.Texture2DArray.ArraySize = 1;
			desc.Texture2DArray.MostDetailedMip = subRes.MipIndex;
			desc.Texture2DArray.MipLevels = subRes.MipSize;
		}
	}
	else if(subDimension == ETextureDimension::Tex2DArray) {
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.FirstArraySlice = subRes.ArrayIndex;
		desc.Texture2DArray.ArraySize = subRes.ArraySize;
		desc.Texture2DArray.MostDetailedMip = subRes.MipIndex;
		desc.Texture2DArray.MipLevels = subRes.MipSize;
	}
	else if(subDimension == ETextureDimension::TexCube) {
		if(texDimension == ETextureDimension::TexCube) {
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			desc.TextureCube.MostDetailedMip = subRes.MipIndex;
			desc.TextureCube.MipLevels = subRes.MipSize;
		}
		else {
			desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
			desc.TextureCubeArray.First2DArrayFace = subRes.ArrayIndex * subPerArraySize;
			desc.TextureCubeArray.NumCubes = 1;
			desc.TextureCubeArray.MostDetailedMip = subRes.MipIndex;
			desc.TextureCubeArray.MipLevels = subRes.MipSize;
		}
	}
	else if (subDimension == ETextureDimension::TexCubeArray) {
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		desc.TextureCubeArray.First2DArrayFace = subRes.ArrayIndex * subPerArraySize;
		desc.TextureCubeArray.NumCubes = subRes.ArraySize;
		desc.TextureCubeArray.MostDetailedMip = subRes.MipIndex;
		desc.TextureCubeArray.MipLevels = subRes.MipSize;
	}
	else if (subDimension == ETextureDimension::Tex3D) {
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MostDetailedMip = subRes.MipIndex;
		desc.Texture3D.MipLevels = subRes.MipSize;
	}
	else {
		LOG_FATAL("[D3D12TextureImpl::CreateSRV] not supported!");
	}
	m_Device->GetDevice()->CreateShaderResourceView(m_Resource, &desc, descriptor);
}

void D3D12TextureImpl::CreateUAV(RHITextureSubRes subRes, D3D12_CPU_DESCRIPTOR_HANDLE descriptor) {
	CHECK(EnumHasAnyFlags(GetDesc().Flags, ETextureFlags::UAV));
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
	desc.Format = ToD3D12ViewFormat(GetDesc().Format, subRes.ViewFlags);
	const ETextureDimension texDimension = GetDesc().Dimension;
	const ETextureDimension subDimension = subRes.Dimension;
	if(subDimension == ETextureDimension::Tex2D && texDimension == ETextureDimension::Tex2D) {
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = subRes.MipIndex;
	}
	else if(subDimension == ETextureDimension::Tex3D) {
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		desc.Texture3D.MipSlice = subRes.MipIndex;
		desc.Texture3D.FirstWSlice = subRes.ArrayIndex;
		desc.Texture3D.WSize = subRes.ArraySize;
	}
	else {
		desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uint32 perArraySize = GetTextureDimension2DSize(subDimension);
		desc.Texture2DArray.FirstArraySlice = subRes.ArrayIndex * perArraySize;
		desc.Texture2DArray.ArraySize = subRes.ArraySize * perArraySize;
		desc.Texture2DArray.MipSlice = subRes.MipIndex;
	}
	m_Device->GetDevice()->CreateUnorderedAccessView(m_Resource, nullptr, &desc, descriptor);
}

void D3D12TextureImpl::CreateRTV(RHITextureSubRes subRes, D3D12_CPU_DESCRIPTOR_HANDLE descriptor) {
	CHECK(EnumHasAnyFlags(GetDesc().Flags, ETextureFlags::ColorTarget));
	// only 1 array slice Texture2D for RTV
	D3D12_RENDER_TARGET_VIEW_DESC desc{};
	desc.Format = ToD3D12ViewFormat(GetDesc().Format, subRes.ViewFlags);
	const ETextureDimension texDimension = GetDesc().Dimension;
	if(texDimension == ETextureDimension::Tex2D) {
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = subRes.MipIndex;
	}
	else {
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.FirstArraySlice = subRes.ArrayIndex;
		desc.Texture2DArray.ArraySize = subRes.ArraySize;
		desc.Texture2DArray.MipSlice = subRes.MipIndex;
	}
	m_Device->GetDevice()->CreateRenderTargetView(m_Resource, &desc, descriptor);
}

void D3D12TextureImpl::CreateDSV(RHITextureSubRes subRes, D3D12_CPU_DESCRIPTOR_HANDLE descriptor) {
	CHECK(EnumHasAnyFlags(GetDesc().Flags, ETextureFlags::DepthStencilTarget));
	D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
	desc.Format = ToD3D12ViewFormat(GetDesc().Format, subRes.ViewFlags);
	const ETextureDimension texDimension = GetDesc().Dimension;
	if (texDimension == ETextureDimension::Tex2D) {
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = subRes.MipIndex;
	}
	else {
		desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.FirstArraySlice = subRes.ArrayIndex;
		desc.Texture2DArray.ArraySize = subRes.ArraySize;
		desc.Texture2DArray.MipSlice = subRes.MipIndex;
	}
	m_Device->GetDevice()->CreateDepthStencilView(m_Resource, &desc, descriptor);
}

D3D12Sampler::D3D12Sampler(const RHISamplerDesc& desc, D3D12Device* device) : RHISampler(desc), m_Device(device){
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_Descriptor = allocator->AllocateDescriptorSlot();
	D3D12_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = ToD3D12Filter(desc.Filter);
	samplerDesc.AddressU = ToD3D12TextureAddressMode(desc.AddressU);
	samplerDesc.AddressV = ToD3D12TextureAddressMode(desc.AddressV);
	samplerDesc.AddressW = ToD3D12TextureAddressMode(desc.AddressW);
	samplerDesc.MinLOD = desc.MinLOD;
	samplerDesc.MaxLOD = desc.MaxLOD;
	samplerDesc.MipLODBias = desc.LODBias;
	samplerDesc.MaxAnisotropy = (UINT)desc.MaxAnisotropy;
	D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle = allocator->GetCPUHandle(m_Descriptor);
	m_Device->GetDevice()->CreateSampler(&samplerDesc, samplerHandle);
}

D3D12Sampler::~D3D12Sampler() {
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	allocator->FreeDescriptorSlot(m_Descriptor);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Sampler::GetCPUHandle() {
	StaticDescriptorAllocator* allocator = m_Device->GetDescriptorMgr()->GetStaticAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	return allocator->GetCPUHandle(m_Descriptor);
}

D3D12Fence::D3D12Fence(ID3D12Device* device): m_TargetValue(0){
	device->CreateFence(m_TargetValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.Address()));
}

void D3D12Fence::SetName(const char* name) {
	const auto nameW = String2WString(name);
	m_Fence->SetName(nameW.c_str());
}

void D3D12Fence::Wait() {
	auto currentValue = m_Fence->GetCompletedValue();
	if(currentValue < m_TargetValue) {
		HANDLE fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		DX_CHECK(m_Fence->SetEventOnCompletion(m_TargetValue, fenceEvent));
		WaitForSingleObject(fenceEvent, 9999);
		CloseHandle(fenceEvent);
	}
}

void D3D12Fence::Reset() {
	ResetValue(0);
}

void D3D12Fence::ResetValue(uint64 val) {
	m_Fence->Signal(val);
	m_TargetValue = val + 1;
}

ID3D12Fence* D3D12Fence::GetFence() {
	return m_Fence.Get();
}

D3D12Shader::D3D12Shader(EShaderStageFlags stage, const void* byteData, uint32 byteSize): RHIShader(stage) {
	m_Bytes.Resize(byteSize);
	memcpy(m_Bytes.Data(), byteData, byteSize);
}

TConstArrayView<uint8> D3D12Shader::GetBytes() {
	return TConstArrayView<uint8>(m_Bytes.Data(), m_Bytes.Size());
}
