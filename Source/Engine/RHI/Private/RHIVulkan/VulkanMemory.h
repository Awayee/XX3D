#pragma once
#include "Core/Public/TypeDefine.h"
#include "VUlkanCommon.h"

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
	void* m_DataPtr{nullptr};
	bool m_Mapped{false};
public:
	~VulkanMem();
	void* Map();
	void Unmap();
	void Free();
};

struct VulkanContext;

class VulkanMemMgr {
public:
	VulkanMemMgr(const VulkanContext* context);
	~VulkanMemMgr();

	bool BindBuffer(VulkanMem& mem, VkBuffer buffer, EMemoryType type, VkMemoryPropertyFlags flags);
	bool BindImage(VulkanMem& mem, VkImage image, EMemoryType type, VkMemoryPropertyFlags flags);

private:
	VmaAllocator m_Allocator = VK_NULL_HANDLE;
};