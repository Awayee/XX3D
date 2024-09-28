#pragma once
#include "D3D12Util.h"
#include "Core/Public/Defines.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TList.h"

// Static descriptor: associating with resource, recycle when resource freed, using for resource setup
// Dynamic descriptor: will reset when frame begin, means that user need allocate every frame, using for pipeline binding.
typedef D3D12_DESCRIPTOR_HEAP_TYPE EStaticDescriptorType;
enum class EDynamicDescriptorType : uint32 {
	CbvSrvUav = 0,
	Sampler,
	Count
};
inline D3D12_CPU_DESCRIPTOR_HANDLE InvalidCPUDescriptor() { return { 0ull }; }
inline D3D12_GPU_DESCRIPTOR_HANDLE InvalidGPUDescriptor() { return { 0ull }; }
EDynamicDescriptorType ToDynamicDescriptorType(EBindingType type);
EStaticDescriptorType ToStaticDescriptorType(EDynamicDescriptorType type);

// a link-list for data reusing
class FreeListAllocator {
public:
	NON_COPYABLE(FreeListAllocator);
	struct Range { uint32 Start, End; };
	FreeListAllocator() = default;
	FreeListAllocator(FreeListAllocator&& rhs) noexcept;
	FreeListAllocator& operator=(FreeListAllocator&& rhs) noexcept;
	uint32 Allocate(uint32 allocSize);// allocate data, return start index, if failed, return DX_INVALID_INDEX
	void Free(uint32 allocStart, uint32 allocSize);// free allocated data, by start index and size
	bool IsEmpty() const;
private:
	TDoubleLinkList<Range> m_Ranges;
};

struct StaticDescriptorHandle {
	uint32 HeapIndex{ DX_INVALID_INDEX };
	uint32 SlotIndex{ DX_INVALID_INDEX };
	bool IsValid() const { return DX_INVALID_INDEX != HeapIndex && DX_INVALID_INDEX != SlotIndex; }
	static StaticDescriptorHandle InValidHandle() { return { DX_INVALID_INDEX, DX_INVALID_INDEX }; }
};

// static descriptor means supporting with allocation and recycling
class StaticDescriptorAllocator {
public:
	NON_COPYABLE(StaticDescriptorAllocator);
	NON_MOVEABLE(StaticDescriptorAllocator);
	StaticDescriptorAllocator(ID3D12Device* device, EStaticDescriptorType type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32 pageSize);
	~StaticDescriptorAllocator() = default;
	StaticDescriptorHandle AllocateDescriptorSlot();
	ID3D12DescriptorHeap* GetHeap(uint32 heapIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(StaticDescriptorHandle descriptor);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(StaticDescriptorHandle descriptor);
	void FreeDescriptorSlot(StaticDescriptorHandle& handle);
private:
	ID3D12Device* m_Device;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	const D3D12_DESCRIPTOR_HEAP_FLAGS m_Flags;
	const uint32 m_PageSize;
	const uint32 m_IncrementSize;
	struct DescriptorHeapStorage {
		TDXPtr<ID3D12DescriptorHeap> Heap;
		FreeListAllocator FreeSlots;
	};
	TArray<DescriptorHeapStorage> m_Heaps;
	FreeListAllocator m_FreeHeaps;
	uint32 m_CurrentHeapIndex;
	void AllocateHeap();
};

struct DynamicDescriptorHandle {
	uint32 HeapIndex{ DX_INVALID_INDEX };
	uint32 SlotIndex{ DX_INVALID_INDEX };
	bool IsValid() const { return DX_INVALID_INDEX != HeapIndex && DX_INVALID_INDEX != SlotIndex; }
	static DynamicDescriptorHandle InvalidHandle() { return { DX_INVALID_INDEX, DX_INVALID_INDEX }; }
};

// Dynamic descriptors will reset when frame begin
class DynamicDescriptorAllocator {
public:
	NON_COPYABLE(DynamicDescriptorAllocator);
	NON_MOVEABLE(DynamicDescriptorAllocator);
	DynamicDescriptorAllocator(ID3D12Device* device, EDynamicDescriptorType type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32 pageSize);
	~DynamicDescriptorAllocator() = default;
	DynamicDescriptorHandle AllocateSlot(uint32 descriptorCount);
	ID3D12DescriptorHeap* GetHeap(uint32 heapIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(DynamicDescriptorHandle handle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(DynamicDescriptorHandle handle);
	uint32 GetIncrementSize() const { return m_IncrementSize; }
	void ResetDescriptors();
private:
	ID3D12Device* m_Device;
	const EDynamicDescriptorType m_Type;
	const D3D12_DESCRIPTOR_HEAP_FLAGS m_Flags;
	const uint32 m_PageSize;
	const uint32 m_IncrementSize;
	struct DescriptorHeapStorage {
		TDXPtr<ID3D12DescriptorHeap> HeapPtr;
		uint32 UsedSlotMax{ 0 };
	};
	TArray<DescriptorHeapStorage> m_Heaps;
	uint32 m_LastAvailableHeap;
	DynamicDescriptorHandle AllocateFromHeap(uint32 heapIndex, uint32 descriptorCount);
	uint32 AllocateHeap();
};

class D3D12DescriptorMgr {
public:
	NON_COPYABLE(D3D12DescriptorMgr);
	NON_MOVEABLE(D3D12DescriptorMgr);
	D3D12DescriptorMgr(ID3D12Device* device);
	~D3D12DescriptorMgr() = default;
	StaticDescriptorAllocator* GetStaticAllocator(EStaticDescriptorType type) { return m_StaticAllocators[EnumCast(type)].Get(); }
	DynamicDescriptorAllocator* GetDynamicAllocator(EDynamicDescriptorType type) { return m_DynamicAllocators[EnumCast(type)].Get(); }
	void BeginFrame();

private:
	TStaticArray<TUniquePtr<StaticDescriptorAllocator>, EnumCast(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)> m_StaticAllocators;
	TStaticArray<TUniquePtr<DynamicDescriptorAllocator>, EnumCast(EDynamicDescriptorType::Count)> m_DynamicAllocators;
};