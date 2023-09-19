#pragma once
#include "Core/Public/String.h"
#include "Core/Public/TRefRange.h"
#include "VulkanCommon.h"

class ContextBuilder {
public:
	enum VulkanInitFlags : uint32 {
		ENABLE_DEBUG=1,
		INTEGRATED_GPU=2,//Discrete GPU is default, use integrated if this enabled
	};
	uint32 Flags;
	XXString AppName{};
	void* WindowHandle;
	TVector<char const*> ValidationLayers{ "VK_LAYER_KHRONOS_validation" };
	TVector<char const*> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	explicit ContextBuilder(VulkanContext& context);
	~ContextBuilder() = default;
	void Build();
	void Release();
private:
	VulkanContext& m_Context;
	TVector<const char*> GetInstanceExtensions();
	void CreateInstance();
	void InitializeDebugger();
	void CreateWindowSurface();
	void PickGPU();
	void CreateDevice();
};

class SwapChainBuilder {
public:
	VkPhysicalDevice PhysicalDevice{ VK_NULL_HANDLE };
	VkSurfaceKHR Surface{ VK_NULL_HANDLE };
	VkDevice Device{ VK_NULL_HANDLE };
	uint32 GraphicsIdx{ INVALID_IDX };
	uint32 PresentIdx{ INVALID_IDX };
	VkExtent2D Extent{ 0, 0 };
	SwapChainBuilder(VkSwapchainKHR& handle, TVector<VkImage>& images, TVector<VkImageView>& imageViews);
	void SetContext(const VulkanContext& context);
	void Build();
private:
	VkSwapchainKHR& m_Handle;
	TVector<VkImage>& m_Images;
	TVector<VkImageView>& m_Views;
};