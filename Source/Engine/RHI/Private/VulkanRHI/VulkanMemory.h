#pragma once
#include "VulkanContext.h"
#include <vk_mem_alloc.h>

class VulkanDevice;
class VulkanMemoryMgr;

enum class EMemoryType : uint8 {
	CPU,
	GPU,
	CPU2GPU,
	GPU2CPU,
};

class BufferAllocation {
public:
	BufferAllocation();
	~BufferAllocation();
	BufferAllocation(const BufferAllocation&) = delete;
	BufferAllocation(BufferAllocation&& rhs) noexcept;
	BufferAllocation& operator=(const BufferAllocation&) = delete;
	BufferAllocation& operator=(BufferAllocation&& rhs) noexcept;
	void* Map();
	void Unmap();
	VkBuffer GetBuffer() const { return m_Buffer; }
	void* GetMappedPointer() { return m_MappedPointer; }
private:
	friend VulkanMemoryMgr;
	void InnerFree();
	VmaAllocator m_Allocator;
	VmaAllocation m_Allocation;
	VkBuffer m_Buffer;
	void* m_MappedPointer;
};

class ImageAllocation {
public:
	ImageAllocation();
	~ImageAllocation();
	ImageAllocation(const ImageAllocation&) = delete;
	ImageAllocation(ImageAllocation&& rhs) noexcept;
	ImageAllocation& operator=(const ImageAllocation&) = delete;
	ImageAllocation& operator=(ImageAllocation&& rhs) noexcept;
private:
	friend VulkanMemoryMgr;
	void InnerFree();
	VmaAllocator m_Allocator{ nullptr };
	VmaAllocation m_Allocation{ nullptr };	
};

class VulkanMemoryMgr {
public:
	VulkanMemoryMgr(const VulkanContext* context, const VulkanDevice* device);
	~VulkanMemoryMgr();
	bool AllocateBufferMemory(BufferAllocation& allocation, uint32 size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryPropertyFlags);
	void FreeBufferMemory(BufferAllocation& allocation);
	bool AllocateImageMemory(ImageAllocation& allocation, VkImage image, VkMemoryPropertyFlags memoryPropertyFlags);
	void FreeImageMemory(ImageAllocation& allocation);
	void Update();
private:
	const VulkanDevice* m_Device = nullptr;
	VmaAllocator m_Allocator = VK_NULL_HANDLE;
};