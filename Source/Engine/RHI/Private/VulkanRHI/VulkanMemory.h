#pragma once
#include "VulkanContext.h"
#include <vk_mem_alloc.h>

class VulkanDevice;
class VulkanMemoryAllocator;

enum class EMemoryType : uint8 {
	CPU,
	GPU,
	CPU2GPU,
	GPU2CPU,
};

class BufferAllocation {
public:
	NON_COPYABLE(BufferAllocation);
	BufferAllocation() = default;
	~BufferAllocation() = default;
	BufferAllocation(BufferAllocation&& rhs) noexcept;
	BufferAllocation& operator=(BufferAllocation&& rhs) noexcept;
	void* Map();
	void Unmap();
	VkBuffer GetBuffer() const { return m_Buffer; }
private:
	friend VulkanMemoryAllocator;
	void InnerFree();
	VmaAllocator m_Allocator{ VK_NULL_HANDLE };
	VmaAllocation m_Allocation{ VK_NULL_HANDLE };
	VkBuffer m_Buffer{ VK_NULL_HANDLE };
};

class ImageAllocation {
public:
	ImageAllocation() = default;
	~ImageAllocation() = default;
	ImageAllocation(const ImageAllocation&) = delete;
	ImageAllocation(ImageAllocation&& rhs) noexcept;
	ImageAllocation& operator=(const ImageAllocation&) = delete;
	ImageAllocation& operator=(ImageAllocation&& rhs) noexcept;
private:
	friend VulkanMemoryAllocator;
	void InnerFree();
	VmaAllocator m_Allocator{ nullptr };
	VmaAllocation m_Allocation{ nullptr };	
};

class VulkanMemoryAllocator {
public:
	NON_COPYABLE(VulkanMemoryAllocator);
	NON_MOVEABLE(VulkanMemoryAllocator);
	VulkanMemoryAllocator(const VulkanContext* context, const VulkanDevice* device);
	~VulkanMemoryAllocator();
	bool AllocateBufferMemory(BufferAllocation& allocation, uint32 size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryPropertyFlags);
	void FreeBufferMemory(BufferAllocation& allocation);
	bool AllocateImageMemory(ImageAllocation& allocation, VkImage image, VkMemoryPropertyFlags memoryPropertyFlags);
	void FreeImageMemory(ImageAllocation& allocation);
private:
	VmaAllocator m_Allocator{ VK_NULL_HANDLE };
};

// dynamic memories will clear when frame beginning, so need allocate every frame.
class VulkanDynamicBufferAllocator {
public:
	struct Allocation {
		uint32 BufferIndex;
		uint32 Offset;
		uint32 Size;
	};
	NON_COPYABLE(VulkanDynamicBufferAllocator);
	NON_MOVEABLE(VulkanDynamicBufferAllocator);
	VulkanDynamicBufferAllocator(VulkanMemoryAllocator* allocator, const VulkanDevice* device, uint32 pageSize);
	~VulkanDynamicBufferAllocator();
	Allocation Allocate(VkBufferUsageFlags usage, uint32 size, const void* data);
	VkBuffer GetBufferHandle(uint32 bufferIndex) const;
	void UnmapAllocations();
	void GC();
private:
	VulkanMemoryAllocator* m_Allocator{ VK_NULL_HANDLE };
	struct BufferChunk {
		BufferAllocation Allocation;
		void* MappedData;
		uint32 AllocatedSize;
	};
	TArray<BufferChunk> m_BufferChunks; // TODO TL
	uint32 m_AllocatedIndex{ VK_INVALID_INDEX };
	uint32 m_UniformAlignment{ 256 };
	uint32 m_PageSize{ 1024 };
	uint32 AllocateChunk();
};