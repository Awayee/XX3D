#pragma once
#include "VulkanCommon.h"
#include "Core/Public/TSingleton.h"
#include <vk_mem_alloc.h>

enum class EMemoryType : uint8 {
	CPU,
	GPU,
	CPU2GPU,
	GPU2CPU,
};



class VulkanMem {
private:
	friend class VulkanMemMgr;
	VmaAllocation m_Handle{ VK_NULL_HANDLE };
	VmaAllocator m_Alloc{ VK_NULL_HANDLE };
	bool m_Mapped{false};
public:
	~VulkanMem();
	void* Map();
	void Unmap();
	void Free();
};

struct VulkanContext;

class VulkanMemMgr: public TSingleton<VulkanMemMgr> {
public:
	bool BindBuffer(VulkanMem& mem, VkBuffer buffer, EMemoryType type, VkMemoryPropertyFlags flags);
	bool BindImage(VulkanMem& mem, VkImage image, EMemoryType type, VkMemoryPropertyFlags flags);
private:
	friend TSingleton<VulkanMemMgr>;
	VmaAllocator m_Allocator = VK_NULL_HANDLE;
	VulkanMemMgr(const VulkanContext* context);
	~VulkanMemMgr();
};