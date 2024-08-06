#pragma once
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "VulkanCommon.h"
#include "VulkanContext.h"

class VulkanMemoryMgr;
class VulkanDescriptorMgr;
class VulkanUploader;
class VulkanCommandContext;

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
	const VkPhysicalDeviceLimits& GetLimits() const { return m_DeviceProperties.limits; }
	const VulkanQueue& GetGraphicsQueue() const { return m_GraphicsQueue; }
	const VulkanQueue& GetComputeQueue() const { return m_ComputeQueue; }
	const VulkanFormats& GetFormats() const { return m_Formats; }

	VulkanMemoryMgr* GetMemoryMgr() { return m_MemoryMgr.Get(); }
	VulkanDescriptorMgr* GetDescriptorMgr() { return m_DescriptorMgr.Get(); }
	VulkanCommandContext* GetCommandContext() { return m_CommandContext.Get(); }
	VulkanUploader* GetUploader() { return m_Uploader.Get(); }

	const VulkanQueue* FindPresentQueue(VkSurfaceKHR surface) const;
private:
	VkPhysicalDevice m_PhysicalDevice{ VK_NULL_HANDLE };
	VkDevice m_Device{ VK_NULL_HANDLE };
	VulkanQueue m_GraphicsQueue;
	VulkanQueue m_ComputeQueue;
	VulkanQueue m_TransferQueue;
	VkPhysicalDeviceProperties m_DeviceProperties{};
	VulkanFormats m_Formats{};
	TUniquePtr<VulkanMemoryMgr> m_MemoryMgr;
	TUniquePtr<VulkanDescriptorMgr> m_DescriptorMgr;
	TUniquePtr<VulkanCommandContext> m_CommandContext;
	TUniquePtr<VulkanUploader> m_Uploader;

	void CreateDevice(const VulkanContext* context);
};