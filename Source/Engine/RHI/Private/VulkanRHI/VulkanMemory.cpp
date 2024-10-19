#include "VulkanMemory.h"
#include "VulkanDevice.h"
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

constexpr uint32 DYNAMIC_PAGE_SIZE = 16 << 10;

inline VmaMemoryUsage ConvertVmaMemoryUsage(EMemoryType type) {
	switch (type) {
	case EMemoryType::CPU:
		return VMA_MEMORY_USAGE_CPU_ONLY;
	case EMemoryType::GPU:
		return VMA_MEMORY_USAGE_GPU_ONLY;
	case EMemoryType::CPU2GPU:
		return VMA_MEMORY_USAGE_CPU_TO_GPU;
	case EMemoryType::GPU2CPU:
		return VMA_MEMORY_USAGE_GPU_TO_CPU;
	}
	return VMA_MEMORY_USAGE_UNKNOWN;
}

inline uint32 Aligned(uint32 size, uint32 alignment) {
	return (size + (alignment - 1)) & ~(alignment - 1);
}

BufferAllocation::BufferAllocation(BufferAllocation&& rhs) noexcept: m_Allocator(rhs.m_Allocator), m_Allocation(rhs.m_Allocation), m_Buffer(rhs.m_Buffer) {
	rhs.InnerFree();
}

BufferAllocation& BufferAllocation::operator=(BufferAllocation&& rhs) noexcept {
	m_Allocation = rhs.m_Allocation;
	m_Allocator = rhs.m_Allocator;
	m_Buffer = rhs.m_Buffer;
	rhs.InnerFree();
	return *this;
}

void* BufferAllocation::Map() {
	void* mappedData;
	vmaMapMemory(m_Allocator, m_Allocation, &mappedData);
	return mappedData;
}

void BufferAllocation::Unmap() {
	vmaUnmapMemory(m_Allocator, m_Allocation);
}

void BufferAllocation::InnerFree() {
	m_Allocation = nullptr;
	m_Allocator = nullptr;
	m_Buffer = nullptr;
}

ImageAllocation::ImageAllocation(ImageAllocation&& rhs) noexcept: m_Allocator(rhs.m_Allocator), m_Allocation(rhs.m_Allocation) {
	rhs.InnerFree();
}

ImageAllocation& ImageAllocation::operator=(ImageAllocation&& rhs) noexcept {
	m_Allocator = rhs.m_Allocator;
	m_Allocation = rhs.m_Allocation;
	rhs.InnerFree();
	return *this;
}

void ImageAllocation::InnerFree() {
	m_Allocator = nullptr;
	m_Allocation = nullptr;
}

VulkanMemoryAllocator::VulkanMemoryAllocator(const VulkanContext* context, const VulkanDevice* device) {
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = context->GetAPIVersion();
	allocatorCreateInfo.instance = context->GetInstance();
	allocatorCreateInfo.physicalDevice = device->GetPhysicalDevice();
	allocatorCreateInfo.device = device->GetDevice();
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
	vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator);
}

VulkanMemoryAllocator::~VulkanMemoryAllocator() {
	vmaDestroyAllocator(m_Allocator);
}

bool VulkanMemoryAllocator::AllocateBufferMemory(BufferAllocation& allocation, uint32 size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryPropertyFlags) {
	VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr, 0 };
	bufferInfo.usage = bufferUsage;
	bufferInfo.size = size;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocationCreateInfo allocationInfo;
	allocationInfo.flags = 0;
	allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocationInfo.requiredFlags = memoryPropertyFlags;
	allocationInfo.preferredFlags = memoryPropertyFlags;
	allocationInfo.memoryTypeBits = 0;
	allocationInfo.pool = nullptr;
	allocationInfo.priority = 1.0f;
	const VkResult result = vmaCreateBuffer(m_Allocator, &bufferInfo, &allocationInfo, &allocation.m_Buffer, &allocation.m_Allocation, nullptr);
	if (VK_SUCCESS == result) {
		allocation.m_Allocator = m_Allocator;
		return true;
	}
	return false;
}

void VulkanMemoryAllocator::FreeBufferMemory(BufferAllocation& allocation) {
	if (allocation.m_Allocator) {
		vmaDestroyBuffer(allocation.m_Allocator, allocation.m_Buffer, allocation.m_Allocation);
		allocation.InnerFree();
	}
}

bool VulkanMemoryAllocator::AllocateImageMemory(ImageAllocation& allocation, VkImage image, VkMemoryPropertyFlags memoryPropertyFlags) {
	VmaAllocationCreateInfo info;
	info.flags = 0;
	info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	info.requiredFlags = memoryPropertyFlags;
	info.preferredFlags = memoryPropertyFlags;
	info.memoryTypeBits = 0;
	info.pool = nullptr;
	info.pUserData = nullptr;
	info.priority = 1.0f;
	if (VK_SUCCESS == vmaAllocateMemoryForImage(m_Allocator, image, &info, &allocation.m_Allocation, nullptr) &&
		VK_SUCCESS == vmaBindImageMemory(m_Allocator, allocation.m_Allocation, image)) {
		allocation.m_Allocator = m_Allocator;
		return true;
	}
	return false;
}

void VulkanMemoryAllocator::FreeImageMemory(ImageAllocation& allocation) {
	if (allocation.m_Allocator) {
		vmaFreeMemory(allocation.m_Allocator, allocation.m_Allocation);
		allocation.InnerFree();
	}
}

VulkanDynamicBufferAllocator::VulkanDynamicBufferAllocator(VulkanMemoryAllocator* allocator, const VulkanDevice* device, uint32 pageSize) :
	m_Allocator(allocator),
	m_UniformAlignment((uint32)device->GetProperties().limits.minUniformBufferOffsetAlignment),
	m_PageSize(pageSize) {}

VulkanDynamicBufferAllocator::~VulkanDynamicBufferAllocator() {
	for(BufferChunk& chunk: m_BufferChunks) {
		m_Allocator->FreeBufferMemory(chunk.Allocation);
	}
}

VulkanDynamicBufferAllocator::Allocation VulkanDynamicBufferAllocator::Allocate(VkBufferUsageFlags usage, uint32 size, const void* data) {
	size = Aligned(size, m_UniformAlignment);
	ASSERT(size <= m_PageSize, "Dynamic allocation size is greater than page size!");
	const uint32 chunkIdx = PrepareChunk(size);
	BufferChunk& chunk = m_BufferChunks[chunkIdx];
	const uint32 offset = chunk.AllocatedSize;
	chunk.AllocatedSize += size;
	if(!chunk.MappedData) {
		chunk.MappedData = chunk.Allocation.Map();
	}
	uint8* mappedPointer = (uint8*)chunk.MappedData + offset;
	memcpy(mappedPointer, data, size);
	return { chunkIdx, offset, size };
}

VkBuffer VulkanDynamicBufferAllocator::GetBufferHandle(uint32 bufferIndex) const {
	return m_BufferChunks[bufferIndex].Allocation.GetBuffer();
}

void VulkanDynamicBufferAllocator::UnmapAllocations() {
	if(VK_INVALID_INDEX != m_AllocatedIndex) {
		// unmap buffers and reset allocations
		for(uint32 i=0; i<=m_AllocatedIndex; ++i) {
			auto& bufferChunk = m_BufferChunks[i];
			if(bufferChunk.MappedData) {
				bufferChunk.Allocation.Unmap();
				bufferChunk.AllocatedSize = 0;
				bufferChunk.MappedData = nullptr;
			}
		}
		m_AllocatedIndex = 0;
	}
}

void VulkanDynamicBufferAllocator::GC() {
	for(; m_BufferChunks.Size() > m_AllocatedIndex+1; m_BufferChunks.PopBack()) {
		m_Allocator->FreeBufferMemory(m_BufferChunks.Back().Allocation);
	}
}

uint32 VulkanDynamicBufferAllocator::PrepareChunk(uint32 requiredSize) {
	if(VK_INVALID_INDEX != m_AllocatedIndex) {
		for (; m_AllocatedIndex < m_BufferChunks.Size(); ++m_AllocatedIndex) {
			if (m_PageSize - m_BufferChunks[m_AllocatedIndex].AllocatedSize >= requiredSize) {
				return m_AllocatedIndex;
			}
		}
	}
	// allocate
	m_AllocatedIndex = m_BufferChunks.Size();
	auto& chunk = m_BufferChunks.EmplaceBack();
	m_Allocator->AllocateBufferMemory(chunk.Allocation, m_PageSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	chunk.AllocatedSize = 0;
	chunk.MappedData = nullptr;
	return m_AllocatedIndex;
}
