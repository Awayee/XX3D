#pragma once
#include "D3D12Util.h"
#include "D3D12Pipeline.h"
#include "D3D12Resources.h"
#include "RHI/Public/RHI.h"
#include "Core/Public/Container.h"

class D3D12Device;

class D3D12StagingBuffer: public D3D12Buffer {
public:
	D3D12StagingBuffer(uint32 byteSize, ID3D12Device* device);
	uint32 GetCreateFrame() const { return m_CreateFrame; }
private:
	uint32 m_CreateFrame;
};

class D3D12Uploader {
public:
	D3D12Uploader(ID3D12Device* device) : m_Device(device) {}
	~D3D12Uploader() = default;
	D3D12Buffer* AllocateStaging(uint32 byteSize);
	void BeginFrame();
private:
	ID3D12Device* m_Device;
	TArray<TUniquePtr<D3D12StagingBuffer>> m_Stagings;
};

class D3D12CommandList: public RHICommandBuffer {
public:
	D3D12CommandList(D3D12Device* device, ID3D12CommandAllocator* alloc, D3D12_COMMAND_LIST_TYPE type);
	~D3D12CommandList() override = default;
	void Reset() override;
	void BeginRendering(const RHIRenderPassInfo& info) override;
	void EndRendering() override;
	void BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) override;
	void BindComputePipeline(RHIComputePipelineState* pipeline) override;
	void SetShaderParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& parameter) override;
	void BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) override;
	void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) override;
	void SetViewport(FRect rect, float minDepth, float maxDepth) override;
	void SetScissor(Rect rect) override;
	void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) override;
	void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) override;
	void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;
	void ClearColorTarget(uint32 targetIndex, const float* color, const IRect& rect) override;
	void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, RHITextureSubRes dstSubRes, IOffset3D dstOffset) override;
	void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region) override;
	void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint32 srcOffset, uint32 dstOffset, uint32 byteSize) override;
	void TransitionTextureState(RHITexture* texture, EResourceState stateBefore, EResourceState stateAfter, RHITextureSubRes subRes) override;
	void GenerateMipmap(RHITexture* texture, uint8 mipSize, uint16 arrayIndex, uint16 arraySize, ETextureViewFlags viewFlags) override;
	void BeginDebugLabel(const char* msg, const float* color) override;
	void EndDebugLabel() override;
	void TransitionResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, uint32 subResIndex);
	void CheckBegin();
	void CheckClose();
	ID3D12GraphicsCommandList* GetD3D12Ptr() { return m_CommandList.Get(); }
private:
	friend class D3D12Queue;
	D3D12Device* m_Device;
	ID3D12CommandAllocator* m_Allocator;
	TDXPtr<ID3D12GraphicsCommandList> m_CommandList;
	TArray<D3D12PipelineDescriptorCache> m_PipelineCaches;
	bool m_IsRecording;
	// call before draw/dispatch
	TMap<D3D12Texture*, ResourceState> m_ResStates;
	void PreDraw();
	void OnSubmit();
};

class D3D12Queue {
public:
	D3D12Queue() = default;
	~D3D12Queue() = default;
	void Initialize(D3D12Device* device, EQueueType type);
	void BeginFrame();
	RHICommandBufferPtr CreateCommandList();
	D3D12CommandList* GetUploadCommand();
	void ExecuteCommandLists(TArrayView<D3D12CommandList*> cmds);
	void SignalFence(D3D12Fence* fence);
	ID3D12CommandQueue* GetCommandQueue() { return m_CmdQueue.Get(); }
private:
	D3D12Device* m_Device;
	D3D12_COMMAND_LIST_TYPE m_CmdType;
	TDXPtr<ID3D12CommandQueue> m_CmdQueue;
	TDXPtr<ID3D12CommandAllocator> m_CommonAllocator;
	TDXPtr<ID3D12CommandAllocator> m_UploadAllocator;
	TUniquePtr<D3D12CommandList> m_UploadCmd;
	uint64 m_CurrentFenceVal;
};

class D3D12CommandMgr{
public:
	D3D12CommandMgr(D3D12Device* device);
	~D3D12CommandMgr() = default;
	void BeginFrame();
	D3D12Queue* GetQueue(EQueueType type);
private:
	D3D12Device* m_Device;
	TStaticArray<D3D12Queue, EnumCast(EQueueType::Count)> m_Queues;
};
