#pragma once
#include "RHIClasses.h"
#include <functional>

namespace RHI{
	typedef void(*DebugFunc)(const char*);
	//typedef void(*CommandBufferFunc)(RCommandBuffer*);
	typedef  std::function<void(RCommandBuffer*)> CommandBufferFunc;

	class RHIInstance
	{
	public:
		virtual void Initialize(const RSInitInfo* initInfo)=0;
		virtual void Release() = 0;
		virtual uint8 GetMaxFramesInFlight() = 0;
		virtual const USize2D& GetSwapchainExtent() = 0;
		virtual RFormat GetSwapchainImageFormat() = 0;
		virtual RImageView* GetSwapchainImageView(uint8 i) = 0;
		virtual uint32 GetSwapchainMaxImageCount() = 0;
		virtual RQueue* GetGraphicsQueue() = 0;
		virtual RFormat GetDepthFormat() = 0;

		virtual void ResizeSwapchain(uint32 width, uint32 height) = 0;

		/**
		 * \brief if RecordBegin is called, some functions will not execute until call RecordEnd, in order to reduce the interaction with graphics API, such as:
		 * RecordBegin();
		 * AllocateDescriptorSet(layout0);
		 * AllocateDescriptorSet(layout1);
		 * AllocateDescriptorSet(Layout2);
		 * RecordEnd();
		 */
		virtual void RecordBegin() = 0;
		virtual void RecordEnd() = 0;

		virtual RRenderPass* CreateRenderPass(uint32 attachmentCount, const RSAttachment* pAttachments,
			uint32 subpassCount, const RSubPassInfo* subpasses,
			uint32 dependencyCount, const RSubpassDependency* dependencies) = 0;
		// for single subpass
		virtual RRenderPass* CreateRenderPass(uint32 colorAttachmentCount, const RSAttachment* pColorAttachments, const RSAttachment* depthAttachment)=0;
		virtual void DestroyRenderPass(RRenderPass* pass) = 0;

		// descriptor set
		virtual RDescriptorSetLayout* CreateDescriptorSetLayout(uint32 bindingCount, const RSDescriptorSetLayoutBinding* bindings) = 0;
		virtual void DestroyDescriptorSetLayout(RDescriptorSetLayout* descriptorSetLayout) = 0;
		virtual RDescriptorSet* AllocateDescriptorSet(const RDescriptorSetLayout* layout) = 0;
		virtual void FreeDescriptorSet(RDescriptorSet* descriptorSet) = 0;
		//virtual void AllocateDescriptorSets(uint32 count, const RDescriptorSetLayout* const* layouts, RDescriptorSet*const* descriptorSets) = 0;
		//virtual void FreeDescriptorSets(uint32 count, RDescriptorSet** descriptorSets) = 0;
		// pipeline
		virtual RPipelineLayout* CreatePipelineLayout(uint32 setLayoutCount, const RDescriptorSetLayout*const* pSetLayouts, uint32 pushConstantRange, const RSPushConstantRange* pPushConstantRanges) = 0;
		virtual void DestroyPipelineLayout(RPipelineLayout* pipelineLayout) = 0;
		virtual RPipeline* CreateGraphicsPipeline(const RGraphicsPipelineCreateInfo& createInfo, RPipelineLayout* layout, RRenderPass* renderPass, uint32 subpass,
			RPipeline* basePipeline, int32_t basePipelineIndex) = 0;
		virtual RPipeline* CreateComputePipeline(const RPipelineShaderInfo& shader, RPipelineLayout* layout, RPipeline* basePipeline, uint32 basePipelineIndex) = 0;
		virtual void DestroyPipeline(RPipeline* pipeline) = 0;

		virtual void QueueSubmit(RQueue* queue, 
			uint32 cmdCount, RCommandBuffer* cmds, 
			uint32 waitSemaphoreCount, RSemaphore* waitSemaphores, RPipelineStageFlags* waitStageFlags, 
			uint32 signalSemaphoreCount, RSemaphore* signalSemaphores, 
			RFence* fence) = 0;
		virtual void QueueWaitIdle(RQueue* queue) = 0;
		virtual void WaitGraphicsQueue() = 0;
		virtual RFramebuffer* CreateFrameBuffer(RRenderPass* pass, uint32 attachmentCount, const RImageView* const* pAttachments, uint32 width, uint32 height, uint32 layers) = 0;
		virtual void DestroyFramebuffer(RFramebuffer* framebuffer) = 0;
		// cmd
		virtual RCommandBuffer* AllocateCommandBuffer(RCommandBufferLevel level) = 0;
		virtual void FreeCommandBuffer(RCommandBuffer* cmd) = 0;
		virtual void ImmediateCommit(const CommandBufferFunc& func) = 0;

		virtual int PreparePresent(uint8 frameIndex) = 0; // return image index of the swapchain, return -1 if out of date.
		virtual int QueueSubmitPresent(RCommandBuffer* cmd, uint8 frameIndex) = 0; // return -1 if out of date

		virtual void FreeMemory(RMemory* memory) = 0;

		// buffer
		virtual RBuffer* CreateBuffer(uint64 size, RBufferUsageFlags usage) = 0;
		virtual RMemory* CreateBufferMemory(RBuffer* buffer, RMemoryPropertyFlags memoryProperty, uint64 dataSize, void* pData) = 0;
		virtual void CreateBufferWithMemory(uint64 size, RBufferUsageFlags usage, RMemoryPropertyFlags memoryFlags,
			RBuffer*& pBuffer, RMemory*& pMemory, uint64 dataSize, void* data) = 0;
		virtual void DestroyBuffer(RBuffer* buffer) = 0;
		virtual void MapMemory(RMemory* memory, void** pData) = 0;
		virtual void UnmapMemory(RMemory* memory) = 0;

		// image
		virtual RImage* CreateImage2D(RFormat format, uint32 width, uint32 height, uint32 mipLevels,
			RSampleCountFlagBits samples, RImageTiling tiling, RImageUsageFlags usage) = 0;
		virtual RMemory* CreateImageMemory(RImage* image, RMemoryPropertyFlags memoryProperty, void* data) = 0;
		virtual void DestroyImage(RImage* image) = 0;
		virtual RImageView* CreateImageView(RImage* image, RImageViewType viewType, RImageAspectFlags aspectMast,
			uint32 baseMiplevel, uint32 levelCount, uint32 baseLayer,uint32 layerCount) = 0;
		virtual void DestroyImageView(RImageView* imageView) = 0;
		virtual RSampler* CreateSampler(const RSSamplerInfo& samplerInfo) = 0;
		virtual void DestroySampler(RSampler* sampler) = 0;

	};

	RHIInstance* GetInstance();
}

#define RHI_INSTANCE RHI::GetInstance()

#define GET_RHI(x) RHI::RHIInstance* x = RHI::GetInstance()
