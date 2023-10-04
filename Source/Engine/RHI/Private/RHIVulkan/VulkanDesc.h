#pragma once
#include "VulkanCommon.h"
#include "Core/Public/Container.h"
#include "Core/Public/TSingleton.h"
#include "RHI/Public/RHIResources.h"

// descriptor set manager
class VulkanDSMgr{
public:
	explicit VulkanDSMgr(VkDevice device);
	~VulkanDSMgr();
	VkDescriptorSetLayout GetLayoutHandle(const RHIShaderBindingLayout& bindingLayout);
	VkDescriptorSet AllocatDS(VkDescriptorSetLayout layout);
private:
	friend TSingleton<VulkanDSMgr>;
	VkDevice m_Device;
	TUnorderedMap<size_t, VkDescriptorSetLayout> m_LayoutMap;
	TVector<VkDescriptorPool> m_Pools;
	void AddPool();
};
