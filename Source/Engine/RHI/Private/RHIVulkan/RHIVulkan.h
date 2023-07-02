#pragma once

#define VK_NO_PROTOTYPES
#ifdef USE_VMA
#include <vk_mem_alloc.h>
#else
#include <vulkan/vulkan.h>
#endif
#include "RHI/Public/RHI.h"
#include "RHIVKResources.h"
#include "RHI/Public/RHIClasses.h"
#include "RHI/Public/RHIStructs.h"
#include "Core/Public/Container.h"
#include "Core/Public/TVector.h"

namespace Engine{
	class RHIVulkan final: public RHI {
#pragma region rhi initialize
	private:
		uint8 m_MaxFramesInFlight{ 3 };
		bool m_EnableValidationLayers{ false };
		bool m_EnableDebugUtils{ false };
		bool m_EnableGeometryShader{ false };
		std::string m_AppName;
		const TVector<char const*> m_ValidationLayers{ "VK_LAYER_KHRONOS_validation" };
		TVector<char const*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		void* m_WindowHandle;

		bool m_Initialized{ false };

		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;
		VkSurfaceKHR m_Surface;
		VkPhysicalDevice m_PhysicalDevice;

		// physical devices's info
		uint32 m_GraphicsIndex;
		uint32 m_PresentIndex;
		uint32 m_ComputeIndex;
		uint32 m_ImageCount;
		RFormat m_SwapchainFormat{FORMAT_UNDEFINED};
		VkColorSpaceKHR m_SwapchainColorSpace;
		VkPresentModeKHR m_SwapchainPresentMode;
		VkSurfaceTransformFlagBitsKHR m_SwapchainTransform;
		VkPhysicalDeviceProperties m_PhysicalDeviceProperties;

		RQueueVk m_GraphicsQueue;
		RQueueVk m_ComputeQueue;
		RQueueVk m_PresentQueue;

		VkDevice m_Device{ nullptr };
		RFormat m_DepthFormat{FORMAT_UNDEFINED};

		VkCommandPool m_RHICommandPool;
		TVector<VkCommandPool> m_CommandPools;
		TVector<RCommandBufferVk> m_CommandBuffers;
		VkDescriptorPool m_DescriptorPool;
		// sync
		TVector<VkSemaphore> m_ImageAvaliableSemaphores;
		TVector<VkSemaphore> m_PresentationFinishSemaphores;
		TVector<VkSemaphore> m_ImageAvaliableForTextureCopySemaphores;
		VmaAllocator m_Vma{ nullptr };


		VkSwapchainKHR m_Swapchain{ nullptr };
		USize2D m_SwapchainExtent;
		uint32 m_CurrentSwapchainImageIndex;
		TVector<VkImage> m_SwapchainImages;
		TVector<RImageViewVk> m_SwapchainImageViews;
		VkRect2D m_Scissor;

		bool m_IsRecording{ false }; // recording mode, some functions will not execute immediately

	private:
		TVector<const char*> GetRequiredExtensions();
		void CreateInstance();
		void InitializeDebugMessenger();
		void CreateWindowSurface();
		void InitializePhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPools();
		void CreateDescriptorPool();
		void CreateSyncResources();
		void CreateMemoryAllocator();
		void CreateSwapchain();
		void ClearSwapchain();
#pragma endregion
		RImageVk* CreateImage(RImageType type, RFormat format, USize3D&& extent, uint32 mipLevels, uint32 arrayLayers, 
			RSampleCountFlags samples, RImageTiling tiling, RImageUsageFlags usage);
	public:
		RHIVulkan() = default;
		void Initialize(const RSInitInfo* initInfo) override;
		void Release() override;
		RSVkImGuiInitInfo GetImGuiInitInfo();
		uint8 GetMaxFramesInFlight() override { return m_MaxFramesInFlight; }
		RFormat GetSwapchainImageFormat() override { return (RFormat)m_SwapchainFormat; }
		const USize2D& GetSwapchainExtent() override { return m_SwapchainExtent; }
		RImageView* GetSwapchainImageView(uint8 i) override;
		uint32 GetSwapchainMaxImageCount() override;
		RQueue* GetGraphicsQueue() override;
		RFormat GetDepthFormat() override { return (RFormat)m_DepthFormat; };

		void ResizeSwapchain(uint32 width, uint32 height) override;

		void RecordBegin() override {};
		void RecordEnd() override {};

		RRenderPass* CreateRenderPass(CRefRange<RSAttachment> attachments, CRefRange<RSubPassInfo> subpasses, CRefRange<RSubpassDependency> dependencies) override;
		RRenderPass* CreateRenderPass(CRefRange<RSAttachment> colorAttachments, const RSAttachment* depthAttachment) override;

		void DestroyRenderPass(RRenderPass* pass) override;

		// descriptor set
		RDescriptorSetLayout* CreateDescriptorSetLayout(CRefRange<RSDescriptorSetLayoutBinding> bindings) override;
		void DestroyDescriptorSetLayout(RDescriptorSetLayout* descriptorSetLayout) override;
		RDescriptorSet* AllocateDescriptorSet(const RDescriptorSetLayout* layout) override;
		//void AllocateDescriptorSets(uint32 count, const RDescriptorSetLayout* const* layouts, RDescriptorSet* const* descriptorSets)override;
		//void FreeDescriptorSets(uint32 count, RDescriptorSet** descriptorSets) override;
		void FreeDescriptorSet(RDescriptorSet* descriptorSet) override;

		// pipeline
		RPipelineLayout* CreatePipelineLayout(CRefRange<RDescriptorSetLayout*> layouts, CRefRange<RSPushConstantRange> pushConstants) override;
		void DestroyPipelineLayout(RPipelineLayout* pipelineLayout) override;
		RPipeline* CreateGraphicsPipeline(const RGraphicsPipelineCreateInfo& createInfo, RPipelineLayout* layout, RRenderPass* renderPass, uint32 subpass,
			RPipeline* basePipeline, int32_t basePipelineIndex) override;
		RPipeline* CreateComputePipeline(const RPipelineShaderInfo& shader, RPipelineLayout* layout, RPipeline* basePipeline, uint32 basePipelineIndex) override;
		void DestroyPipeline(RPipeline* pipeline) override;

		// command buffer
		void QueueSubmit(RQueue* queue, CRefRange<RCommandBuffer*> cmds, CRefRange<RSemaphore*> waitSemaphores, CRefRange<RPipelineStageFlags> waitStageFlags, CRefRange<RSemaphore*> signalSemaphores, RFence* fence) override;
		void QueueWaitIdle(RQueue* queue)override;
		RFramebuffer* CreateFrameBuffer(RRenderPass* pass, CRefRange<RImageView*> attachments, uint32 width, uint32 height, uint32 layers) override;
		void DestroyFramebuffer(RFramebuffer* framebuffer) override;
		RCommandBuffer* AllocateCommandBuffer(RCommandBufferLevel level)override;
		void FreeCommandBuffer(RCommandBuffer* cmd) override;

		void ImmediateCommit(const CommandBufferFunc& func) override;
		int PreparePresent(uint8 frameIndex) override;
		int QueueSubmitPresent(RCommandBuffer* cmd, uint8 frameIndex, RFence* fence) override;

		// buffer
		RBuffer* CreateBuffer(uint64 size, RBufferUsageFlags usage) override;
		RMemory* CreateBufferMemory(RBuffer* buffer, RMemoryPropertyFlags memoryProperty, uint64 dataSize, void* pData) override;
		void CreateBufferWithMemory(uint64 size, RBufferUsageFlags usage, RMemoryPropertyFlags memoryFlags,
			RBuffer*& pBuffer, RMemory*& pMemory, uint64 dataSize, void* pData)override;
		void DestroyBuffer(RBuffer* buffer) override;
		void MapMemory(RMemory* memory, void** pData) override;
		void UnmapMemory(RMemory* memory) override;

		// image
		RImage* CreateImage2D(RFormat format, uint32 width, uint32 height, uint32 mipLevels,
		                      RSampleCountFlagBits samples, RImageTiling tiling, RImageUsageFlags usage) override;
		RMemory* CreateImageMemory(RImage* image, RMemoryPropertyFlags memoryProperty, void* pData) override;
		void DestroyImage(RImage* image) override;
		RImageView* CreateImageView(RImage* image, RImageViewType viewType, RImageAspectFlags aspectMask,
			uint32 baseMipLevel, uint32 levelCount, uint32 baseLayer, uint32 layerCount) override;
		void DestroyImageView(RImageView* imageView) override;
		RSampler* CreateSampler(const RSSamplerInfo& samplerInfo) override;
		void DestroySampler(RSampler* sampler) override;\

		//sync
		RFence* CreateFence(bool sig) override;
		void DestroyFence(RFence* fence) override;
		void WaitForFence(RFence* fence) override;
		void ResetFence(RFence* fence) override;


		void FreeMemory(RMemory* memory)override;
		VkDevice GetDevice();
		static RHIVulkan* InstanceVulkan();
	};
}