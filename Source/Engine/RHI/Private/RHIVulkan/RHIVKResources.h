#pragma once
#include "RHI/Public/RHIClasses.h"
#include "VUlkanFuncs.h"
#ifdef USE_VMA
#include <vk_mem_alloc.h>
#else
#include <vulkan/vulkan.h>
#endif
namespace Engine {
#define RESOURCE_VK_HANDLE(cls, vkHandle)\
	class cls##Vk{\
	public:\
		vkHandle handle;\
	}

	class RHIVulkan;

	struct RSVkImGuiInitInfo {
		void* windowHandle;
		VkInstance instance;
		VkDevice device;
		VkPhysicalDevice physicalDevice;
		uint32 queueIndex;
		VkQueue queue;
		VkDescriptorPool descriptorPool;
	};

	class RWindowHandleVk: public RWindowHandle {
	public:
		uint64_t handle;
	};


	class RRenderPassVk: public RRenderPass {
	public:
		VkRenderPass handle;
		void SetAttachment(uint32 idx, RImageView* imageView) override;
		void SetClearValue(uint32 idx, const RSClear& clear) override;
		const TVector<VkClearValue>& GetClears() { return m_Clears; }
	private:
		TVector<VkClearValue> m_Clears;
		TVector<VkImageView> m_Attachments;
		friend RHIVulkan;
	};

	class RMemoryVk: public RMemory {
	public:
		VkDeviceMemory handle;
		VkDeviceSize offset;
		VkDeviceSize size;
	};

	class RMemoryVma: public RMemory {
	public:
		VmaAllocation handle;
	};

	class RImageVk: public RImage {
	public:
		friend RHIVulkan;
		VkImage handle;
		void SetLayout(RImageLayout layout) { m_Layout = layout; }
	};

	class RImageViewVk: public RImageView {
	public:
		VkImageView handle;
		friend RHIVulkan;
	};

	class RFramebufferVk: public RFramebuffer {
	public:
		VkFramebuffer handle;
		friend RHIVulkan;
	};

	class RQueueVk: public RQueue {
	public:
		VkQueue handle;
	};

	class RSemaphoreVk: public RSemaphore {
	public:
		VkSemaphore handle;
	};

	class RFenceVk: public RFence {
	public:
		VkFence handle;
	};

	class RBufferVk: public RBuffer {
		friend RHIVulkan;
	public:
		VkBuffer handle;
	};

	class RSamplerVk: public RSampler {
	public:
		VkSampler handle;
	};

	class RPipelineVk: public RPipeline {
	public:
		VkPipeline handle;
		friend RHIVulkan;
	};

	class RDescriptorSetLayoutVk: public RDescriptorSetLayout {
	public:
		VkDescriptorSetLayout handle;
	};

	class RPipelineLayoutVk : public RPipelineLayout {
	public:
		VkPipelineLayout handle;
	};

	class RDescriptorSetVk: public RDescriptorSet {
		friend RHIVulkan;
	private:
		void InnerUpdate(uint32 binding, uint32 arrayElement, uint32 count, VkDescriptorType type, const VkDescriptorImageInfo* imageInfo, const VkDescriptorBufferInfo* bufferInfo, const VkBufferView* texelBufferView);
	public:
		VkDescriptorSet handle;
		void Update(uint32 binding, RDescriptorType type, const RDescriptorInfo& info, uint32 arrayElement, uint32 count) override;
		void SetUniformBuffer(uint32 binding, RBuffer* buffer) override;
		void SetImageSampler(uint32 binding, RSampler* sampler, RImageView* image) override;
		void SetImage(uint32 binding, RImageView* image) override;
		void SetSampler(uint32 binding, RSampler* sampler) override;
		void SetInputAttachment(uint32 binding, RImageView* image) override;
	};

	class RCommandBufferVk : public RCommandBuffer {
	public:
		RCommandBufferVk() = default;
		VkCommandBuffer handle{ VK_NULL_HANDLE };
		VkCommandPool m_Pool{ VK_NULL_HANDLE };
		void Begin(RCommandBufferUsageFlags flags) override;
		void End() override;
		void BeginRenderPass(RRenderPass* pass, RFramebuffer* framebuffer, const URect& area) override;
		void NextSubpass() override;
		void EndRenderPass() override;
		void CopyBufferToImage(RBuffer* buffer, RImage* image, RImageAspectFlags aspect, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) override;
		void BlitImage(RCommandBuffer* cmd, RImage* srcImage, RImage* dstImage, const RSImageBlit& region) override;
		void BindPipeline(RPipeline* pipeline) override;
		void BindDescriptorSet(RPipelineLayout* layout, RDescriptorSet* descriptorSet, uint32 setIdx, RPipelineType pipelineType) override;
		void BindVertexBuffer(RBuffer* buffer, uint32 first, uint64 offset) override;
		void BindIndexBuffer(RBuffer* buffer, uint64 offset) override;
		void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) override;
		void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) override;
		void DrawPrimitive(RBuffer* buffer, uint32 vertexCount, uint32 instanceCount) override;
		void DrawPrimitiveIndexed(RBuffer* vertexBuffer, RBuffer* indexBuffer, uint32 indexCount, uint32 instanceCount) override;
		void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;
		void ClearAttachment(RImageAspectFlags aspect, const float* color, const URect& rect) override;
		void CopyBuffer(RBuffer* srcBuffer, RBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) override;
		void TransitionImageLayout(RImage* image, RImageLayout oldLayout, RImageLayout newLayout, uint32 baseMipLevel, uint32 levelCount, uint32 baseLayer, uint32 layerCount, RImageAspectFlags aspect) override;
		void GenerateMipmap(RImage* image, uint32 levelCount, RImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount) override;
		void BeginDebugLabel(const char* msg, const float* color) override;
		void EndDebugLabel() override;
	};

}