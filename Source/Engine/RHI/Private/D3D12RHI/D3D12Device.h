#pragma once
#include "D3D12Util.h"

class D3D12DescriptorMgr;
class D3D12CommandMgr;
class D3D12Uploader;

class D3D12DynamicBufferAllocator {
public:
	struct Allocation {
		uint32 BufferIndex;
		uint32 Offset;
		uint32 Size;
	};
	D3D12DynamicBufferAllocator(ID3D12Device* device, uint32 pageSize);
	~D3D12DynamicBufferAllocator() = default;
	Allocation Allocate(EBufferFlags flags, uint32 size, const void* data);
	ID3D12Resource* GetResource(uint32 bufferIndex);
	void UnmapAllocations();
private:
	ID3D12Device* m_Device;
	struct BufferChunk {
		TDXPtr<ID3D12Resource> Buffer;
		void* MappedData;
		uint32 AllocatedSize;
	};
	TArray<BufferChunk> m_BufferChunks;
	uint32 m_PageSize;
	uint32 m_AllocatedIndex;
	uint32 AllocateChunk();
};

class D3D12Device {
public:
	D3D12Device(IDXGIAdapter* adapter);
	~D3D12Device() = default;
	D3D12DescriptorMgr* GetDescriptorMgr() { return m_DescriptorMgr.Get(); }
	D3D12CommandMgr* GetCommandMgr() { return m_CommandMgr.Get(); }
	D3D12Uploader* GetUploader() { return m_Uploader.Get(); }
	D3D12DynamicBufferAllocator* GetDynamicMemoryAllocator() { return m_DynamicMemoryAllocator.Get(); }
	ID3D12Device* GetDevice() { return m_Device; }
private:
	TDXPtr<ID3D12Device> m_Device;
	TUniquePtr<D3D12DescriptorMgr> m_DescriptorMgr;
	TUniquePtr<D3D12Uploader> m_Uploader;
	TUniquePtr<D3D12CommandMgr> m_CommandMgr;
	TUniquePtr<D3D12DynamicBufferAllocator> m_DynamicMemoryAllocator;
};