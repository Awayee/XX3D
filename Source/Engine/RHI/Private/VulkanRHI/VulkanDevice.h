#pragma once
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "VulkanCommon.h"
#include "VulkanContext.h"
#include "RHI/Public/RHI.h"

class VulkanDescriptorSetMgr;
class VulkanUploader;
class VulkanCommandContext;
class VulkanMemoryAllocator;
class VulkanDynamicBufferAllocator;

struct VulkanQueue {
	VkQueue Handle{ VK_NULL_HANDLE };
	uint32 FamilyIndex{ UINT32_MAX };
	uint32 QueueIndex{ 0 };
};

struct VulkanFormats {
	VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
};

class VulkanDevice {
public:
	explicit VulkanDevice(const VulkanContext* context, VkPhysicalDevice physicalDevice);
	~VulkanDevice();
	VkDevice GetDevice() const { return m_Device; }
	VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
	const VkPhysicalDeviceProperties& GetProperties() const  { return m_DeviceProperties; }
	const VulkanQueue& GetQueue(EQueueType queue) const { return m_Queues[EnumCast(queue)]; }
	const VulkanFormats& GetFormats() const { return m_Formats; }
	VulkanMemoryAllocator* GetMemoryAllocator() { return m_MemoryAllocator.Get(); }
	VulkanDynamicBufferAllocator* GetDynamicBufferAllocator() { return m_DynamicBufferAllocator.Get(); }
	VulkanDescriptorSetMgr* GetDescriptorMgr() { return m_DescriptorMgr.Get(); }
	VulkanCommandContext* GetCommandContext() { return m_CommandContext.Get(); }
	VulkanUploader* GetUploader() { return m_Uploader.Get(); }

	const VulkanQueue* FindPresentQueue(VkSurfaceKHR surface) const;
private:
	VkPhysicalDevice m_PhysicalDevice{ VK_NULL_HANDLE };
	VkDevice m_Device{ VK_NULL_HANDLE };
	TStaticArray<VulkanQueue, EnumCast(EQueueType::Count)> m_Queues;
	VkPhysicalDeviceProperties m_DeviceProperties{};
	VulkanFormats m_Formats{};
	TUniquePtr<VulkanMemoryAllocator> m_MemoryAllocator;
	TUniquePtr<VulkanDynamicBufferAllocator> m_DynamicBufferAllocator;
	TUniquePtr<VulkanDescriptorSetMgr> m_DescriptorMgr;
	TUniquePtr<VulkanCommandContext> m_CommandContext;
	TUniquePtr<VulkanUploader> m_Uploader;
	void InitializeDeviceInfo();
	void CreateDevice(const VulkanContext* context);
};