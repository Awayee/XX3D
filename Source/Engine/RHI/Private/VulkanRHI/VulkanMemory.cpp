#include "VulkanMemory.h"
#include "VulkanDevice.h"
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

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

BufferAllocation::BufferAllocation(): m_Allocator(nullptr), m_Allocation(nullptr), m_Buffer(nullptr), m_MappedPointer(nullptr){
}

BufferAllocation::~BufferAllocation() {
}

BufferAllocation::BufferAllocation(BufferAllocation&& rhs) noexcept: m_Allocator(rhs.m_Allocator), m_Allocation(rhs.m_Allocation), m_Buffer(rhs.m_Buffer), m_MappedPointer(rhs.m_MappedPointer) {
	rhs.InnerFree();
}

BufferAllocation& BufferAllocation::operator=(BufferAllocation&& rhs) noexcept {
	m_Allocation = rhs.m_Allocation;
	m_Allocator = rhs.m_Allocator;
	m_Buffer = rhs.m_Buffer;
	m_MappedPointer = rhs.m_MappedPointer;
	rhs.InnerFree();
	return *this;
}

void* BufferAllocation::Map() {
	ASSUME(!m_MappedPointer);
	vmaMapMemory(m_Allocator, m_Allocation, &m_MappedPointer);
	return m_MappedPointer;
}

void BufferAllocation::Unmap() {
	ASSUME(m_MappedPointer);
	vmaUnmapMemory(m_Allocator, m_Allocation);
	m_MappedPointer = nullptr;
}

void BufferAllocation::InnerFree() {
	m_Allocation = nullptr;
	m_Allocator = nullptr;
	m_Buffer = nullptr;
	m_MappedPointer = nullptr;
}

ImageAllocation::ImageAllocation() {
}

ImageAllocation::~ImageAllocation() {
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

bool VulkanMemoryMgr::AllocateBufferMemory(BufferAllocation& allocation, uint32 size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryPropertyFlags) {
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
	if(VK_SUCCESS == result) {
		allocation.m_Allocator = m_Allocator;
		return true;
	}
	return false;
}

void VulkanMemoryMgr::FreeBufferMemory(BufferAllocation& allocation) {
	if(allocation.m_Allocator) {
		vmaFreeMemory(allocation.m_Allocator, allocation.m_Allocation);
		vkDestroyBuffer(m_Device->GetDevice(), allocation.m_Buffer, nullptr);
		allocation.InnerFree();
	}
}

bool VulkanMemoryMgr::AllocateImageMemory(ImageAllocation& allocation, VkImage image, VkMemoryPropertyFlags memoryPropertyFlags) {
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

void VulkanMemoryMgr::FreeImageMemory(ImageAllocation& allocation) {
	if (allocation.m_Allocator) {
		vmaFreeMemory(allocation.m_Allocator, allocation.m_Allocation);
		allocation.InnerFree();
	}
}

void VulkanMemoryMgr::Update() {
}

VulkanMemoryMgr::VulkanMemoryMgr(const VulkanContext* context, const VulkanDevice* device): m_Device(device) {

	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = context->GetAPIVersion();
	allocatorCreateInfo.instance = context->GetInstance();
	allocatorCreateInfo.physicalDevice = m_Device->GetPhysicalDevice();
	allocatorCreateInfo.device = m_Device->GetDevice();
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
	vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator);
}

VulkanMemoryMgr::~VulkanMemoryMgr() {
	vmaDestroyAllocator(m_Allocator);
}
