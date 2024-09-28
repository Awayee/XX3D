#include "D3D12Device.h"
#include "D3D12Descriptor.h"
#include "D3D12Command.h"

D3D12DynamicBufferAllocator::D3D12DynamicBufferAllocator(ID3D12Device* device, uint32 pageSize) : m_Device(device), m_PageSize(pageSize), m_AllocatedIndex(DX_INVALID_INDEX) {}

D3D12DynamicBufferAllocator::Allocation D3D12DynamicBufferAllocator::Allocate(EBufferFlags flags, uint32 size, const void* data) {
	size = AlignConstantBufferSize(size);
	ASSERT(size <= m_PageSize, "Dynamic allocation size is greater than page size!");
	if (DX_INVALID_INDEX == m_AllocatedIndex || (m_PageSize - m_BufferChunks[m_AllocatedIndex].AllocatedSize) < size) {
		m_AllocatedIndex = AllocateChunk();
	}
	BufferChunk& chunk = m_BufferChunks[m_AllocatedIndex];
	const uint32 offset = chunk.AllocatedSize;
	chunk.AllocatedSize += size;
	if (!chunk.MappedData) {
		DX_CHECK(chunk.Buffer->Map(0, nullptr, &chunk.MappedData));
	}
	uint8* mappedPointer = (uint8*)chunk.MappedData + offset;
	memcpy(mappedPointer, data, size);
	return { m_AllocatedIndex, offset, size };
}

ID3D12Resource* D3D12DynamicBufferAllocator::GetResource(uint32 bufferIndex) {
	CHECK(bufferIndex < m_BufferChunks.Size());
	return m_BufferChunks[bufferIndex].Buffer;
}

void D3D12DynamicBufferAllocator::UnmapAllocations() {
	if (DX_INVALID_INDEX != m_AllocatedIndex) {
		// unmap buffers and reset allocations
		for (uint32 i = 0; i <= m_AllocatedIndex; ++i) {
			m_BufferChunks[i].Buffer->Unmap(0, nullptr);
			m_BufferChunks[i].MappedData = nullptr;
			m_BufferChunks[i].AllocatedSize = 0;
		}
		m_AllocatedIndex = 0;
	}
}

uint32 D3D12DynamicBufferAllocator::AllocateChunk() {
	uint32 chunkIndex = m_BufferChunks.Size();
	auto& chunk = m_BufferChunks.EmplaceBack();
	auto d3d12Desc = CD3DX12_RESOURCE_DESC::Buffer(m_PageSize);
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
	DX_CHECK(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &d3d12Desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(chunk.Buffer.Address())));
	chunk.AllocatedSize = 0;
	chunk.MappedData = nullptr;
	return chunkIndex;
}

D3D12Device::D3D12Device(IDXGIAdapter* adapter) {
	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_Device.Address()));
	if (FAILED(hardwareResult)) {
		LOG_ERROR("Falied to create D3D12 device!");
	}
	m_DescriptorMgr.Reset(new D3D12DescriptorMgr(m_Device));
	m_DynamicMemoryAllocator.Reset(new D3D12DynamicBufferAllocator(m_Device, RHI_DYNAMIC_BUFFER_PAGE));
	m_Uploader.Reset(new D3D12Uploader(m_Device));
	m_CommandMgr.Reset(new D3D12CommandMgr(this));
}
