#include "D3D12Device.h"
#include "D3D12Descriptor.h"
#include "D3D12Command.h"

D3D12DynamicBufferAllocator::D3D12DynamicBufferAllocator(ID3D12Device* device, uint32 pageSize) : m_Device(device), m_PageSize(pageSize), m_AllocatedIndex(DX_INVALID_INDEX) {}

RHIDynamicBuffer D3D12DynamicBufferAllocator::Allocate(EBufferFlags flags, uint32 size, const void* data, uint32 stride) {
	size = AlignConstantBufferSize(size);
	ASSERT(size <= m_PageSize, "Dynamic allocation size is greater than page size!");
	const uint32 chunkIdx = PrepareChunk(size);
	BufferChunk& chunk = m_BufferChunks[chunkIdx];
	const uint32 offset = chunk.AllocatedSize;
	chunk.AllocatedSize += size;
	if (!chunk.MappedData) {
		DX_CHECK(chunk.Buffer->Map(0, nullptr, &chunk.MappedData));
	}
	uint8* mappedPointer = (uint8*)chunk.MappedData + offset;
	memcpy(mappedPointer, data, size);
	return { chunkIdx, offset, size, stride };
}

ID3D12Resource* D3D12DynamicBufferAllocator::GetResource(uint32 bufferIndex) {
	CHECK(bufferIndex < m_BufferChunks.Size());
	return m_BufferChunks[bufferIndex].Buffer;
}

void D3D12DynamicBufferAllocator::CreateCBV(const RHIDynamicBuffer& dBuffer, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle) {
	ID3D12Resource* resource = m_BufferChunks[dBuffer.BufferIndex].Buffer;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = resource->GetGPUVirtualAddress() + dBuffer.Offset;
	cbvDesc.SizeInBytes = dBuffer.Size;
	m_Device->CreateConstantBufferView(&cbvDesc, descriptorHandle);
}

void D3D12DynamicBufferAllocator::CreateUAV(const RHIDynamicBuffer& dBuffer, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle) {
	ID3D12Resource* resource = m_BufferChunks[dBuffer.BufferIndex].Buffer;
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	// if stride == 0, as raw buffer, otherwise as structured buffer
	if (dBuffer.Stride == 0) {
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		uavDesc.Buffer.FirstElement = dBuffer.Offset / sizeof(uint32);
		uavDesc.Buffer.NumElements = dBuffer.Size / sizeof(uint32);
		uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	}
	else {
		uavDesc.Buffer.StructureByteStride = dBuffer.Stride;
		uavDesc.Buffer.FirstElement = dBuffer.Offset / dBuffer.Stride;
		uavDesc.Buffer.NumElements = dBuffer.Size / dBuffer.Stride;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	}
	m_Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, descriptorHandle);
}

D3D12_VERTEX_BUFFER_VIEW D3D12DynamicBufferAllocator::CreateVertexBufferView(const RHIDynamicBuffer& dBuffer) {
	ID3D12Resource* resource = m_BufferChunks[dBuffer.BufferIndex].Buffer;
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = resource->GetGPUVirtualAddress() + dBuffer.Offset;
	vbv.SizeInBytes = dBuffer.Size;
	vbv.StrideInBytes = dBuffer.Stride;
	return vbv;
}

void D3D12DynamicBufferAllocator::UnmapAllocations() {
	if (DX_INVALID_INDEX != m_AllocatedIndex) {
		// unmap buffers and reset allocations
		for (uint32 i = 0; i <= m_AllocatedIndex; ++i) {
			auto& bufferChunk = m_BufferChunks[i];
			if(bufferChunk.MappedData) {
				bufferChunk.Buffer->Unmap(0, nullptr);
				bufferChunk.MappedData = nullptr;
				bufferChunk.AllocatedSize = 0;
			}
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

uint32 D3D12DynamicBufferAllocator::PrepareChunk(uint32 requiredSize) {
	if (DX_INVALID_INDEX != m_AllocatedIndex) {
		for (; m_AllocatedIndex < m_BufferChunks.Size(); ++m_AllocatedIndex) {
			if (m_PageSize - m_BufferChunks[m_AllocatedIndex].AllocatedSize >= requiredSize) {
				return m_AllocatedIndex;
			}
		}
	}
	// allocate
	m_AllocatedIndex = m_BufferChunks.Size();
	auto& chunk = m_BufferChunks.EmplaceBack();
	auto d3d12Desc = CD3DX12_RESOURCE_DESC::Buffer(m_PageSize);
	const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
	DX_CHECK(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &d3d12Desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(chunk.Buffer.Address())));
	chunk.AllocatedSize = 0;
	chunk.MappedData = nullptr;
	return m_AllocatedIndex;
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
