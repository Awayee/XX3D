#include "VulkanMemory.h"
#include "VulkanBuilder.h"
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

VulkanMem::~VulkanMem() {
	Free();
}

void* VulkanMem::Map() {
	void* dataPtr;
	vmaMapMemory(m_Alloc, m_Handle, &dataPtr);
	return dataPtr;
}

void VulkanMem::Unmap() {
	vmaUnmapMemory(m_Alloc, m_Handle);
}

void VulkanMem::Free() {
	if(m_Handle) {
		if(m_Mapped) {
			Unmap();
		}
		vmaFreeMemory(m_Alloc, m_Handle);
		m_Handle = VK_NULL_HANDLE;
		m_Alloc = VK_NULL_HANDLE;
	}
}

bool VulkanMemMgr::BindBuffer(VulkanMem& mem, VkBuffer buffer, EMemoryType type, VkMemoryPropertyFlags flags) {
	VmaAllocationCreateInfo info;
	info.flags = 0;
	info.usage = ConvertVmaMemoryUsage(type);
	info.requiredFlags = flags;
	info.preferredFlags = flags;
	info.memoryTypeBits = 0;
	info.pool = nullptr;
	info.priority = 1.0f;
	if (VK_SUCCESS != vmaAllocateMemoryForBuffer(m_Allocator, buffer, &info, &mem.m_Handle, nullptr) ||
		VK_SUCCESS != vmaBindBufferMemory(m_Allocator, mem.m_Handle, buffer)) {
		return false;
	}
	mem.m_Alloc = m_Allocator;
	return true;
}

bool VulkanMemMgr::BindImage(VulkanMem& mem, VkImage image, EMemoryType type, VkMemoryPropertyFlags flags) {
	VmaAllocationCreateInfo info;
	info.flags = 0;
	info.usage = ConvertVmaMemoryUsage(type);
	info.requiredFlags = flags;
	info.preferredFlags = flags;
	info.memoryTypeBits = 0;
	info.pool = nullptr;
	info.priority = 1.0f;
	if (VK_SUCCESS != vmaAllocateMemoryForImage(m_Allocator, image, &info, &mem.m_Handle, nullptr) ||
		VK_SUCCESS != vmaBindImageMemory(m_Allocator, mem.m_Handle, image)) {
		return false;
	}
	mem.m_Alloc = m_Allocator;
	return true;
}

VulkanMemMgr::VulkanMemMgr(const VulkanContext* context) {

	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = context->APIVersion;
	allocatorCreateInfo.physicalDevice = context->PhysicalDevice;
	allocatorCreateInfo.device = context->Device;
	allocatorCreateInfo.instance = context->Instance;
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
	vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator);
}

VulkanMemMgr::~VulkanMemMgr() {
	vmaDestroyAllocator(m_Allocator);
}
