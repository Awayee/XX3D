#include "D3D12command.h"
#include "D3D12Device.h"
#include "D3D12Resources.h"
#include "System/Public/FrameCounter.h"
#include "Math/Public/Math.h"
#include <WinPixEventRuntime/pix3.h>

D3D12StagingBuffer::D3D12StagingBuffer(uint32 byteSize, ID3D12Device* device): D3D12Buffer(RHIBufferDesc{EBufferFlags::CopySrc, byteSize}, device) {
	m_CreateFrame = FrameCounter::GetFrame();
}

D3D12Buffer* D3D12Uploader::AllocateStaging(uint32 byteSize) {
	return m_Stagings.EmplaceBack(new D3D12StagingBuffer(byteSize, m_Device)).Get();
}

void D3D12Uploader::BeginFrame() {
	// check and release resource
	const uint32 frame = FrameCounter::GetFrame();
	for (uint32 i = 0; i < m_Stagings.Size(); ) {
		const uint32 createFrame = m_Stagings[i]->GetCreateFrame();
		if (createFrame + RHI_FRAME_IN_FLIGHT_MAX < frame) {
			m_Stagings.SwapRemoveAt(i);
		}
		else {
			++i;
		}
	}
}

D3D12CommandList::D3D12CommandList(D3D12Device* device, ID3D12CommandAllocator* alloc, D3D12_COMMAND_LIST_TYPE type) : m_Device(device), m_Allocator(alloc), m_IsRecording(false) {
	DX_CHECK(device->GetDevice()->CreateCommandList(0, type, m_Allocator, nullptr, IID_PPV_ARGS(m_CommandList.Address())));
	m_CommandList->Close();
}

void D3D12CommandList::Reset() {
	if(!m_IsRecording) {
		m_CommandList->Reset(m_Allocator, nullptr);
		m_PipelineCaches.Reset();
		m_ResStates.clear();
		m_IsRecording = true;
	}
}

void D3D12CommandList::BeginRendering(const RHIRenderPassInfo& info) {
	// color target
	const uint32 colorTargetSize = info.GetNumColorTargets();
	TFixedArray<D3D12_CPU_DESCRIPTOR_HANDLE> colorTargets(colorTargetSize);
	for(uint32 i=0; i<colorTargetSize; ++i) {
		const auto& rt = info.ColorTargets[i];
		if(!rt.Target) {
			break;
		}
		RHITextureSubRes subRes{ rt.ArrayIndex, 1, rt.MipIndex, 1, ETextureDimension::Tex2D, ETextureViewFlags::Color };
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = ((D3D12Texture*)rt.Target)->GetDescriptor(ETexDescriptorType::RTV, subRes);
		if(ERTLoadOption::Clear == rt.LoadOp) {
			m_CommandList->ClearRenderTargetView(rtvHandle, &rt.ColorClear.r, 0, nullptr); // 0 rect means clear entire view
		}
		colorTargets[i] = rtvHandle;
	}

	// depth stencil target
	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilTarget = InvalidCPUDescriptor();
	const auto& ds = info.DepthStencilTarget;
	if(ds.Target) {
		RHITextureSubRes subRes{ ds.ArrayIndex, 1, ds.MipIndex, 1, ETextureDimension::Tex2D, ETextureViewFlags::DepthStencil };
		depthStencilTarget = ((D3D12Texture*)ds.Target)->GetDescriptor(ETexDescriptorType::DSV, subRes);
		D3D12_CLEAR_FLAGS dsClearFlags = (D3D12_CLEAR_FLAGS)0;
		if(ERTLoadOption::Clear == ds.DepthLoadOp) {
			dsClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
		}
		if(ERTLoadOption::Clear == ds.StencilLoadOp) {
			dsClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
		}
		if(dsClearFlags) {
			m_CommandList->ClearDepthStencilView(depthStencilTarget, dsClearFlags, ds.DepthClear, ds.StencilClear, 0, nullptr);
		}
	}
	m_CommandList->OMSetRenderTargets(colorTargetSize, colorTargets.Data(), false, depthStencilTarget.ptr ? &depthStencilTarget : nullptr);
	const D3D12_VIEWPORT viewport{ (float)info.RenderArea.x, (float)info.RenderArea.y, (float)info.RenderArea.w, (float)info.RenderArea.h, 0.0f, 1.0f };
	m_CommandList->RSSetViewports(1, &viewport);
	const D3D12_RECT scissor{ (LONG)info.RenderArea.x, (LONG)info.RenderArea.y, (LONG)info.RenderArea.w, (LONG)info.RenderArea.h };
	m_CommandList->RSSetScissorRects(1, &scissor);
	CheckBegin();
}

void D3D12CommandList::EndRendering() {
	// do nothing
}

void D3D12CommandList::BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) {
	auto* d3d12Pipeline = (D3D12GraphicsPipelineState*)pipeline;
	m_CommandList->SetPipelineState(d3d12Pipeline->GetPipelineState());
	m_CommandList->SetGraphicsRootSignature(d3d12Pipeline->GetRootSignature());
	m_CommandList->IASetPrimitiveTopology(d3d12Pipeline->GetPrimitiveTopology());
	m_PipelineCaches.PushBack(D3D12PipelineDescriptorCache(m_Device, d3d12Pipeline));
}

void D3D12CommandList::BindComputePipeline(RHIComputePipelineState* pipeline) {
	auto* d3d12Pipeline = (D3D12ComputePipelineState*)pipeline;
	m_CommandList->SetPipelineState(d3d12Pipeline->GetPipelineState());
	m_CommandList->SetComputeRootSignature(d3d12Pipeline->GetRootSignature());
	m_PipelineCaches.PushBack(D3D12PipelineDescriptorCache(m_Device, d3d12Pipeline));
}

void D3D12CommandList::SetShaderParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& parameter) {
	if(m_PipelineCaches.Size()) {
		m_PipelineCaches.Back().SetShaderParam(setIndex, bindIndex, parameter);
	}
}

void D3D12CommandList::BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) {
	D3D12_VERTEX_BUFFER_VIEW vb = ((D3D12BufferImpl*)buffer)->GetVertexBufferView();
	m_CommandList->IASetVertexBuffers(first, 1, &vb);
}

void D3D12CommandList::BindIndexBuffer(RHIBuffer* buffer, uint64 offset) {
	D3D12_INDEX_BUFFER_VIEW ib = ((D3D12BufferImpl*)buffer)->GetIndexBufferView();
	m_CommandList->IASetIndexBuffer(&ib);
}

void D3D12CommandList::SetViewport(FRect rect, float minDepth, float maxDepth) {
	D3D12_VIEWPORT d3d12Rect{ rect.x, rect.y, rect.w, rect.h, minDepth, maxDepth };
	m_CommandList->RSSetViewports(1, &d3d12Rect);
}

void D3D12CommandList::SetScissor(Rect rect) {
	D3D12_RECT d3d12Rect{ rect.x, rect.y, (LONG)rect.w, (LONG)rect.h };
	m_CommandList->RSSetScissorRects(1, &d3d12Rect);
}

void D3D12CommandList::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) {
	PreDraw();
	m_CommandList->DrawInstanced(vertexCount, instanceCount, firstIndex, firstInstance);
}

void D3D12CommandList::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) {
	PreDraw();
	m_CommandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, (INT)vertexOffset, firstInstance);
}

void D3D12CommandList::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) {
	PreDraw();
	m_CommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void D3D12CommandList::ClearColorTarget(uint32 targetIndex, const float* color, const IRect& rect) {
	// TODO
}

void D3D12CommandList::CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, RHITextureSubRes dstSubRes, IOffset3D dstOffset) {
	ID3D12Resource* d3d12Buffer = ((D3D12BufferImpl*)buffer)->GetResource();
	ID3D12Resource* d3d12Texture = ((D3D12Texture*)texture)->GetResource();
	const auto& texDesc = texture->GetDesc();
	const uint8 mip = dstSubRes.MipIndex;
	const uint32 cpyWidth = texDesc.GetMipLevelWidth(mip), cpyHeight = texDesc.GetMipLevelHeight(mip);
	const uint32 rowPitch = AlignTexturePitchSize(cpyWidth * texDesc.GetPixelByteSize());
	const uint32 slicePitch = AlignTextureSliceSize(rowPitch * cpyHeight);
	const uint32 perSliceSize = GetD3D12PerArraySliceSize(dstSubRes.Dimension);
	const uint32 arrayIndex = dstSubRes.ArrayIndex * perSliceSize;
	const uint32 arraySize = perSliceSize;
	for(uint16 arr=0; arr<arraySize; ++arr) {
		D3D12_TEXTURE_COPY_LOCATION srcLocation{};
		srcLocation.pResource = d3d12Buffer;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint.Offset = arr * slicePitch;
		srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srcLocation.PlacedFootprint.Footprint.Width = cpyWidth;
		srcLocation.PlacedFootprint.Footprint.Height = cpyHeight;
		srcLocation.PlacedFootprint.Footprint.Depth = texDesc.Depth;
		srcLocation.PlacedFootprint.Footprint.RowPitch = rowPitch;
		D3D12_TEXTURE_COPY_LOCATION dstLocation{};
		dstLocation.pResource = d3d12Texture;
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLocation.SubresourceIndex = texDesc.MipSize * (arrayIndex + arr) + dstSubRes.MipIndex;
		m_CommandList->CopyTextureRegion(&dstLocation, dstOffset.x, dstOffset.y, dstOffset.z, &srcLocation, nullptr);
	}
}

void D3D12CommandList::CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region) {
	ID3D12Resource* d3d12SrcTex = ((D3D12Texture*)srcTex)->GetResource();
	ID3D12Resource* d3d12DstTex = ((D3D12Texture*)dstTex)->GetResource();
	const auto& srcTexDesc = srcTex->GetDesc(), dstTexDesc = dstTex->GetDesc();
	const auto& srcSubRes = region.SrcSubRes, dstSubRes = region.DstSubRes;
	const auto& srcOffset = region.SrcOffset, dstOffset = region.DstOffset;
	const auto& extent = region.Extent;
	const uint32 srcPerArraySize = GetD3D12PerArraySliceSize(srcSubRes.Dimension);
	const uint32 dstPerArraySize = GetD3D12PerArraySliceSize(srcSubRes.Dimension);
	const uint32 cpyArraySize = Math::Min<uint32>(srcSubRes.ArraySize * srcPerArraySize, dstSubRes.ArraySize * dstPerArraySize);
	for(uint32 i = 0; i<cpyArraySize; ++i) {
		D3D12_TEXTURE_COPY_LOCATION srcLocation{};
		srcLocation.pResource = d3d12SrcTex;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcLocation.SubresourceIndex = srcTexDesc.MipSize * (srcSubRes.ArrayIndex * srcPerArraySize + i) + srcSubRes.MipIndex;
		D3D12_TEXTURE_COPY_LOCATION dstLocation{};
		dstLocation.pResource = d3d12DstTex;
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLocation.SubresourceIndex = dstTexDesc.MipSize * (dstSubRes.ArrayIndex * srcPerArraySize + i) + dstSubRes.MipIndex;
		D3D12_BOX srcBox{ srcOffset.x, srcOffset.y, srcOffset.z,
			srcOffset.x + extent.w, srcOffset.y + extent.h, srcOffset.z + extent.d };
		auto desc1 = d3d12SrcTex->GetDesc();
		m_CommandList->CopyTextureRegion(&dstLocation, dstOffset.x, dstOffset.y, dstOffset.z, &srcLocation, &srcBox);
	}
}

void D3D12CommandList::CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint32 srcOffset, uint32 dstOffset, uint32 byteSize) {
	ID3D12Resource* d3d12SrcBuffer = ((D3D12BufferImpl*)srcBuffer)->GetResource();
	ID3D12Resource* d3d12DstBuffer = ((D3D12BufferImpl*)dstBuffer)->GetResource();
	m_CommandList->CopyBufferRegion(d3d12DstBuffer, dstOffset, d3d12SrcBuffer, srcOffset, byteSize);
}

void D3D12CommandList::TransitionTextureState(RHITexture* texture, EResourceState stateBefore, EResourceState stateAfter, RHITextureSubRes subRes) {
	D3D12Texture* d3d12Texture = (D3D12Texture*)texture;
	ID3D12Resource* resource = d3d12Texture->GetResource();
	const auto& texDesc = texture->GetDesc();
	D3D12_RESOURCE_STATES d3d12StateBefore;
	//d3d12StateBefore = ToD3D12ResourceState(stateBefore);
	const D3D12_RESOURCE_STATES d3d12StateAfter  = ToD3D12ResourceState(stateAfter);

	// get resource state
	ResourceState* resState;
	if (auto iter = m_ResStates.find(d3d12Texture); iter != m_ResStates.end()) {
		resState = &iter->second;
	}
	else {
		// first fetch the texture, so copy the ResourceState object from texture.
		const ResourceState& resStateFromTex = d3d12Texture->GetResState();
		resState = &m_ResStates.try_emplace(d3d12Texture, ResourceState{ resStateFromTex }).first->second;
	}
	CHECK(resState);

	if(texDesc.IsAllSubRes(subRes)) {
		d3d12StateBefore = resState->GetSubResState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		if(d3d12StateBefore != d3d12StateAfter) {
			TransitionResourceState(resource, d3d12StateBefore, d3d12StateAfter, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
			resState->SetState(d3d12StateAfter);
		}
	}
	else {
		const uint32 barrierCount = subRes.ArraySize * subRes.MipSize;
		TArray<D3D12_RESOURCE_BARRIER> barriers(barrierCount);
		for (uint32 arr = 0; arr < subRes.ArraySize; ++arr) {
			for (uint32 mip = 0; mip < subRes.MipSize; ++mip) {
				const uint32 arrayIndex = subRes.ArrayIndex + arr;
				const uint32 mipIndex = subRes.MipIndex + mip;
				const uint32 subResIndex = D3D12CalcSubresource(mipIndex, arrayIndex, 0, texDesc.MipSize, texDesc.ArraySize);
				d3d12StateBefore = resState->GetSubResState(subResIndex);
				if(d3d12StateBefore != d3d12StateAfter) {
					barriers[arr * subRes.MipSize + mip] = CD3DX12_RESOURCE_BARRIER::Transition(resource, d3d12StateBefore, d3d12StateAfter, subResIndex);
					resState->SetSubResState(subResIndex, d3d12StateAfter);
				}
			}
		}
		m_CommandList->ResourceBarrier(barrierCount, barriers.Data());
	}
}

void D3D12CommandList::GenerateMipmap(RHITexture* texture, uint8 mipSize, uint16 arrayIndex, uint16 arraySize, ETextureViewFlags viewFlags) {
}

void D3D12CommandList::BeginDebugLabel(const char* msg, const float* color) {
	PIXBeginEvent(m_CommandList.Get(), 0xffffffff, msg);
}

void D3D12CommandList::EndDebugLabel() {
	PIXEndEvent(m_CommandList.Get());
}

void D3D12CommandList::TransitionResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, uint32 subResIndex) {
	const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, stateBefore, stateAfter, subResIndex);
	m_CommandList->ResourceBarrier(1, &barrier);
}

void D3D12CommandList::CheckBegin() {
	if(!m_IsRecording) {
		Reset();
	}
}

void D3D12CommandList::CheckClose() {
	if(m_IsRecording) {
		m_CommandList->Close();
		m_IsRecording = false;
	}
}

void D3D12CommandList::PreDraw() {
	if(m_PipelineCaches.Size()) {
		m_PipelineCaches.Back().PreDraw(m_CommandList.Get());
	}
}

void D3D12CommandList::OnSubmit() {
	// check the  resource state and write to texture
	for(auto& [tex, resStateOnCmd] : m_ResStates) {
		ResourceState& resStateOnTex = tex->GetResState();
		resStateOnTex = resStateOnCmd;
	}
	m_ResStates.clear();
}

void D3D12Queue::Initialize(D3D12Device* device, EQueueType type) {
	m_CmdQueue.Reset();
	m_CommonAllocator.Reset();
	m_UploadAllocator.Reset();
	m_Device = device;
	m_CmdType = ToD3D12CommandListType(type);
	ID3D12Device* d3d12Device = device->GetDevice();
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = m_CmdType;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_CmdQueue.Address()));
	d3d12Device->CreateCommandAllocator(m_CmdType, IID_PPV_ARGS(m_CommonAllocator.Address()));
	d3d12Device->CreateCommandAllocator(m_CmdType, IID_PPV_ARGS(m_UploadAllocator.Address()));
	m_UploadCmd.Reset(new D3D12CommandList(device, m_UploadAllocator.Get(), m_CmdType));
	m_CurrentFenceVal = 0;
	// DEBUG set queue name
	switch(type) {
	case EQueueType::Graphics:
		m_CmdQueue->SetName(L"GraphicsQueue");
		break;
	case EQueueType::Compute:
		m_CmdQueue->SetName(L"ComputeQueue");
		break;
	case EQueueType::Transfer:
		m_CmdQueue->SetName(L"TransferQueue");
		break;
	}
}

void D3D12Queue::BeginFrame() {
	m_CommonAllocator->Reset();
	if(m_UploadCmd->m_IsRecording) {
		m_UploadCmd->CheckClose();
		auto* cmd = m_UploadCmd.Get();
		ExecuteCommandLists({cmd});
	}
	else {
		m_UploadAllocator->Reset();
	}
	m_CurrentFenceVal = 0;
}

RHICommandBufferPtr D3D12Queue::CreateCommandList() {
	return RHICommandBufferPtr(new D3D12CommandList(m_Device, m_CommonAllocator, m_CmdType));
}

D3D12CommandList* D3D12Queue::GetUploadCommand() {
	m_UploadCmd->CheckBegin();
	return m_UploadCmd;
}

void D3D12Queue::ExecuteCommandLists(TArrayView<D3D12CommandList*> cmds) {
	TArray<ID3D12CommandList*> d3dCmds; d3dCmds.Reserve(cmds.Size());
	for (D3D12CommandList* cmd : cmds) {
		cmd->CheckClose();
		cmd->OnSubmit();
		d3dCmds.PushBack(cmd->GetD3D12Ptr());
	}
	m_CmdQueue->ExecuteCommandLists(d3dCmds.Size(), d3dCmds.Data());
}

void D3D12Queue::SignalFence(D3D12Fence* fence) {
	fence->ResetValue(m_CurrentFenceVal++);
	DX_CHECK(m_CmdQueue->Signal(fence->GetFence(), fence->GetTargetValue()));
}

D3D12CommandMgr::D3D12CommandMgr(D3D12Device* device) : m_Device(device){
	for(uint32 i=0; i<m_Queues.Size(); ++i) {
		m_Queues[i].Initialize(m_Device, (EQueueType)i);
	}
}

void D3D12CommandMgr::BeginFrame() {
	for (auto& queue : m_Queues) {
		queue.BeginFrame();
	}
}

D3D12Queue* D3D12CommandMgr::GetQueue(EQueueType type) {
	return &m_Queues[EnumCast(type)];
}

