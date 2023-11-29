#pragma once
#include "Core/Public/String.h"
#include "Core/Public/TArrayView.h"
#include "Core/Public/TVector.h"
#include "VulkanCommon.h"

class ContextBuilder {
public:
	struct SDesc {
		XXString AppName{};
		bool EnableDebug{ false };
		bool IntegratedGPU{ false };//Discrete GPU is default, use integrated GPU if this flag is enabled
		void* WindowHandle{ nullptr };
		VkSurfaceFormatKHR SurfaceFormat{ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		TVector<char const*> ValidationLayers{ "VK_LAYER_KHRONOS_validation" };
		TVector<char const*> DeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	};
	SDesc Desc;
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
	struct SDesc {
		VkPhysicalDevice PhysicalDevice{ VK_NULL_HANDLE };
		VkSurfaceKHR Surface{ VK_NULL_HANDLE };
		VkDevice Device{ VK_NULL_HANDLE };
		uint32 GraphicsIdx{ INVALID_IDX };
		uint32 PresentIdx{ INVALID_IDX };
		VkSurfaceFormatKHR SwapchainFormat{ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		VkExtent2D Extent{ 0, 0 };
		SDesc() = default;
		SDesc(const VulkanContext& context);
	};
	SDesc Desc;
	SwapChainBuilder(VkSwapchainKHR& handle, TVector<VkImage>& images, TVector<VkImageView>& imageViews);
	void Build();
private:
	VkSwapchainKHR& m_Handle;
	TVector<VkImage>& m_Images;
	TVector<VkImageView>& m_Views;
};