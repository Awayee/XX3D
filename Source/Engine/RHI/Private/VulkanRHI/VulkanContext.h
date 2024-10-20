#pragma once
#include "VulkanCommon.h"
#include "System/Public/Configuration.h"

class VulkanContext {
public:
	explicit VulkanContext(bool enableDebug, uint32 apiVersion);
	~VulkanContext();
	VkPhysicalDevice PickPhysicalDevice();
	uint32 GetAPIVersion() const { return m_APIVersion; }
	VkInstance GetInstance() const { return m_Instance; }
private:
	friend class VulkanRHI;
	uint32 m_APIVersion;
	VkInstance m_Instance{VK_NULL_HANDLE};
	VkDebugUtilsMessengerEXT m_DebugUtilsMessenger{VK_NULL_HANDLE};
	// Initialize must be called at begin or after Release
	void CreateInstance(bool enableDebug);
	void CreateDebugUtilsMessenger();
};