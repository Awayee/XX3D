#include "D3D12Descriptor.h"

constexpr uint32 UNIFORM_PAGE_SAGE = 64;

EDynamicDescriptorType ToDynamicDescriptorType(EBindingType type) {
	switch (type) {
	case EBindingType::UniformBuffer:
	case EBindingType::StorageBuffer:
	case EBindingType::StorageTexture:
	case EBindingType::Texture: return EDynamicDescriptorType::CbvSrvUav;
	case EBindingType::Sampler: return EDynamicDescriptorType::Sampler;
	default: return EDynamicDescriptorType::Count;
	}
}

EStaticDescriptorType ToStaticDescriptorType(EDynamicDescriptorType type) {
	switch(type) {
	case EDynamicDescriptorType::CbvSrvUav: return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	case EDynamicDescriptorType::Sampler: return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	default: return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	}
}


FreeListAllocator::FreeListAllocator(FreeListAllocator&& rhs) noexcept : m_Ranges(MoveTemp(rhs.m_Ranges)) {} 

FreeListAllocator& FreeListAllocator::operator=(FreeListAllocator&& rhs) noexcept {
	m_Ranges = MoveTemp(rhs.m_Ranges);
	return *this;
}

uint32 FreeListAllocator::Allocate(uint32 allocSize) {
	auto* node = m_Ranges.Head();
	if (!node) {
		return DX_INVALID_INDEX;
	}
	Range& range = node->Value;
	if (range.End - range.Start < allocSize) {
		return DX_INVALID_INDEX;
	}
	const uint32 result = range.Start;
	range.Start += allocSize;
	if (range.Start >= range.End) {
		m_Ranges.RemoveNode(m_Ranges.Head());
	}
	return result;
}

void FreeListAllocator::Free(uint32 allocStart, uint32 allocSize) {
	bool bFound = false;
	for (auto* node = m_Ranges.Head(); node; node = node->Next) {
		Range& range = node->Value;
		CHECK(range.Start < range.End);
		// check if equal to start or end
		if (range.Start == (allocStart + allocSize)) {
			range.Start = allocStart;
			bFound = true;
		}
		else if (range.End == allocStart) {
			range.End += allocSize;
			bFound = true;
		}
		else {
			// check if less than range
			CHECK(range.End < allocStart || range.Start > allocStart);
			if (range.Start > allocStart) {
				m_Ranges.InsertBefore({ allocStart, allocStart + allocSize }, node);
				bFound = true;
			}
		}
		if (bFound) {
			break;
		}
	}
	// otherwise insert at the right
	if (!bFound) {
		m_Ranges.InsertAfterTail({ allocStart, allocStart + allocSize });
	}
}

bool FreeListAllocator::IsEmpty() const {
	return m_Ranges.Size() == 0;
}

StaticDescriptorAllocator::StaticDescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32 pageSize):
m_Device(device),
m_Type(type),
m_Flags(flags),
m_PageSize(pageSize),
m_IncrementSize(device->GetDescriptorHandleIncrementSize(type)),
m_CurrentHeapIndex(DX_INVALID_INDEX){}

StaticDescriptorHandle StaticDescriptorAllocator::AllocateDescriptorSlot() {
	uint32 slotIndex = DX_INVALID_INDEX;
	if(DX_INVALID_INDEX != m_CurrentHeapIndex) {
		slotIndex = m_Heaps[m_CurrentHeapIndex].FreeSlots.Allocate(1);
	}
	if(DX_INVALID_INDEX == slotIndex) {
		AllocateHeap();
		slotIndex = m_Heaps[m_CurrentHeapIndex].FreeSlots.Allocate(1);
	}
	return { m_CurrentHeapIndex, slotIndex };
}

ID3D12DescriptorHeap* StaticDescriptorAllocator::GetHeap(uint32 heapIndex) {
	return m_Heaps[heapIndex].Heap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE StaticDescriptorAllocator::GetCPUHandle(StaticDescriptorHandle descriptor) {
	if (!descriptor.IsValid()) {
		return InvalidCPUDescriptor();
	}
	DescriptorHeapStorage& heapData = m_Heaps[descriptor.HeapIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heapData.Heap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += (size_t)(descriptor.SlotIndex * m_IncrementSize);
	return cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE StaticDescriptorAllocator::GetGPUHandle(StaticDescriptorHandle descriptor) {
	if(!descriptor.IsValid()) {
		return InvalidGPUDescriptor();
	}
	DescriptorHeapStorage& heapData = m_Heaps[descriptor.HeapIndex];
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = heapData.Heap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += (size_t)(descriptor.SlotIndex * m_IncrementSize);
	return gpuHandle;
}

void StaticDescriptorAllocator::FreeDescriptorSlot(StaticDescriptorHandle& handle){
	if(!handle.IsValid()) {
		return;
	}
	DescriptorHeapStorage& heapData = m_Heaps[handle.HeapIndex];
	heapData.FreeSlots.Free(handle.SlotIndex, 1);
	if (heapData.FreeSlots.IsEmpty()) {
		m_FreeHeaps.Free(handle.HeapIndex, 1);
	}
	handle = StaticDescriptorHandle::InValidHandle();
}

void StaticDescriptorAllocator::AllocateHeap() {
	m_CurrentHeapIndex = m_FreeHeaps.Allocate(1);
	if(DX_INVALID_INDEX != m_CurrentHeapIndex) {
		return;
	}
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_PageSize;
	heapDesc.Type = m_Type;
	heapDesc.Flags = m_Flags;
	uint32 newIndex = m_Heaps.Size();
	auto& heapData = m_Heaps.EmplaceBack();
	DX_CHECK(m_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(heapData.Heap.Address())));
	// new space as freed
	heapData.FreeSlots.Free(0, m_PageSize);
	m_FreeHeaps.Free(newIndex, 1);
	m_CurrentHeapIndex = m_FreeHeaps.Allocate(1);
}

DynamicDescriptorAllocator::DynamicDescriptorAllocator(ID3D12Device* device, EDynamicDescriptorType type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32 pageSize) :
	m_Device(device),
	m_Type(type),
	m_Flags(flags),
	m_PageSize(pageSize),
	m_IncrementSize(device->GetDescriptorHandleIncrementSize(ToStaticDescriptorType(m_Type))),
	m_LastAvailableHeap(DX_INVALID_INDEX) {}

DynamicDescriptorHandle DynamicDescriptorAllocator::AllocateSlot(uint32 descriptorCount) {
	CHECK(descriptorCount < m_PageSize);
	if(DX_INVALID_INDEX != m_LastAvailableHeap){
		for (uint32 heapIndex = m_LastAvailableHeap; heapIndex < m_Heaps.Size(); ++heapIndex) {
			if(DynamicDescriptorHandle handle = AllocateFromHeap(heapIndex, descriptorCount); handle.IsValid()) {
				return handle;
			}
		}
	}
	return AllocateFromHeap(AllocateHeap(), descriptorCount); // from allocated new heap directly
}

ID3D12DescriptorHeap* DynamicDescriptorAllocator::GetHeap(uint32 heapIndex) {
	return m_Heaps[heapIndex].HeapPtr.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DynamicDescriptorAllocator::GetCPUHandle(DynamicDescriptorHandle handle) {
	if(!handle.IsValid()) {
		return InvalidCPUDescriptor();
	}
	ID3D12DescriptorHeap* heap = m_Heaps[handle.HeapIndex].HeapPtr;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += (size_t)(handle.SlotIndex * m_IncrementSize);
	return cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorAllocator::GetGPUHandle(DynamicDescriptorHandle handle) {
	if (!handle.IsValid()) {
		return InvalidGPUDescriptor();
	}
	ID3D12DescriptorHeap* heap = m_Heaps[handle.HeapIndex].HeapPtr;
	D3D12_GPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetGPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += (size_t)(handle.SlotIndex * m_IncrementSize);
	return cpuHandle;
}

void DynamicDescriptorAllocator::ResetDescriptors() {
	if (!m_Heaps.IsEmpty()) {
		for(auto& heap: m_Heaps) {
			heap.UsedSlotMax = 0;
		}
		m_LastAvailableHeap = 0;
	}
}

DynamicDescriptorHandle DynamicDescriptorAllocator::AllocateFromHeap(uint32 heapIndex, uint32 descriptorCount) {
	auto& heap = m_Heaps[heapIndex];
	if(m_PageSize - heap.UsedSlotMax < descriptorCount) {
		return DynamicDescriptorHandle::InvalidHandle();
	}
	CHECK(heap.HeapPtr->GetDesc().Type == ToStaticDescriptorType(m_Type));
	DynamicDescriptorHandle handle{ heapIndex, heap.UsedSlotMax };
	heap.UsedSlotMax += descriptorCount;
	return handle;
}

uint32 DynamicDescriptorAllocator::AllocateHeap() {
	if (DX_INVALID_INDEX == m_LastAvailableHeap) {
		m_LastAvailableHeap = m_Heaps.Size();
	}
	uint32 heapIndex = m_Heaps.Size();
	DescriptorHeapStorage& newHeap = m_Heaps.EmplaceBack();
	newHeap.UsedSlotMax = 0;
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.NumDescriptors = m_PageSize;
	heapDesc.Type = ToStaticDescriptorType(m_Type);
	heapDesc.Flags = m_Flags;
	m_Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(newHeap.HeapPtr.Address()));
	// Update last available heap index
	for (; m_LastAvailableHeap < m_Heaps.Size() - 1; ++m_LastAvailableHeap) {
		if (m_PageSize - m_Heaps[m_LastAvailableHeap].UsedSlotMax > 0) {
			break;
		}
	}
	return heapIndex;
}


D3D12DescriptorMgr::D3D12DescriptorMgr(ID3D12Device* device){
	for(int i=0; i<D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
		m_StaticAllocators[i].Reset(new StaticDescriptorAllocator(device, (D3D12_DESCRIPTOR_HEAP_TYPE)i, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, UNIFORM_PAGE_SAGE));
	}
	for(int i=0; i<EnumCast(EDynamicDescriptorType::Count); ++i) {
		m_DynamicAllocators[i].Reset(new DynamicDescriptorAllocator(device, (EDynamicDescriptorType)i, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, UNIFORM_PAGE_SAGE * 4));
	}
}

void D3D12DescriptorMgr::BeginFrame() {
	for(auto& gpuAllocator: m_DynamicAllocators) {
		gpuAllocator->ResetDescriptors();
	}
}
