#pragma once
#include "RHI/Public/RHIClasses.h"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

struct VulkanContext;

namespace Engine {
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

	class RWindowHandleVk : public RWindowHandle {
	public:
		uint64_t handle;
	};

	class RSemaphoreVk: public RSemaphore {
	public:
		VkSemaphore handle;
	};

	class RRenderPassVk : public RRenderPass {
	public:
		VkRenderPass handle;
		void SetAttachment(uint32 idx, RHITexture* texture) override;
		void SetClearValue(uint32 idx, const RSClear& clear) override;
		const TVector<VkClearValue>& GetClears() { return m_Clears; }
	private:
		TVector<VkClearValue> m_Clears;
		TVector<VkImageView> m_Attachments;
		friend RHIVulkan;
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
		void SetUniformBuffer(uint32 binding, RHIBuffer* buffer) override;
		void SetImageSampler(uint32 binding, RHISampler* sampler, RHITexture* texture) override;
		void SetTexture(uint32 binding, RHITexture* texture) override;
		void SetSampler(uint32 binding, RHISampler* sampler) override;
		void SetInputAttachment(uint32 binding, RHITexture* texture) override;
	};

	class RHIVulkanCommandBuffer : public RHICommandBuffer {
	public:
		RHIVulkanCommandBuffer(VkCommandBuffer cmd, VkCommandPool pool): m_VkCmd(cmd), m_Pool(pool){}
		~RHIVulkanCommandBuffer() override;
		void Begin(RCommandBufferUsageFlags flags) override;
		void End() override;
		void BeginRenderPass(RRenderPass* pass, RFramebuffer* framebuffer, const URect& area) override;
		void NextSubpass() override;
		void EndRenderPass() override;
		void CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) override;
		void CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture,uint32 mipLevel, uint32 baseLayer, uint32 layerCount) override;
		void CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RSTextureCopyRegion& region) override;
		void TransitionTextureLayout(RHITexture* texture, RImageLayout oldLayout, RImageLayout newLayout, uint32 baseMipLevel, uint32 levelCount, uint32 baseLayer, uint32 layerCount, RImageAspectFlags aspect) override;
		void GenerateMipmap(RHITexture* texture, uint32 levelCount, RImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount) override;
		void BindPipeline(RPipeline* pipeline) override;
		void BindDescriptorSet(RPipelineLayout* layout, RDescriptorSet* descriptorSet, uint32 setIdx, RPipelineType pipelineType) override;
		void BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) override;
		void BindIndexBuffer(RHIBuffer* buffer, uint64 offset) override;
		void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) override;
		void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) override;
		void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;
		void ClearAttachment(RImageAspectFlags aspect, const float* color, const URect& rect) override;
		void BeginDebugLabel(const char* msg, const float* color) override;
		void EndDebugLabel() override;
	private:
		VkCommandBuffer m_VkCmd{ VK_NULL_HANDLE };
		VkCommandPool m_Pool{ VK_NULL_HANDLE };
	};

}