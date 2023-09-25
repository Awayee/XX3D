#pragma once
#include "VulkanCommon.h"

struct VulkanContext;
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
