#pragma once
#include "VulkanCommon.h"
#include "Core/Public/Container.h"
#include "Core/Public/TSingleton.h"
#include "RHI/Public/RHIResources.h"

using namespace Engine;

class VulkanLayoutMgr : public TSingleton<VulkanLayoutMgr>{
public:
	VkDescriptorSetLayout GetLayoutHandle(const RHIBindingLayout& bindingLayout);
private:
	friend TSingleton<VulkanLayoutMgr>;
	VkDevice m_Device;
	TUnorderedMap<size_t, VkDescriptorSetLayout> m_LayoutMap;
	explicit VulkanLayoutMgr(VkDevice device);
	~VulkanLayoutMgr() override;
};
