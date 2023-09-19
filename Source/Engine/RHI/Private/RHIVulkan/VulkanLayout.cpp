#include "VulkanLayout.h"
#include "Core/Public/Algorithm.h"
#include "VulkanConverter.h"

inline size_t GetLayoutHash(const RHIBindingLayout& bindingLayout) {
	size_t byteSize = sizeof(bindingLayout[0]) * bindingLayout.Size();
	const uint8* bytes = reinterpret_cast<const uint8*>(bindingLayout.Data());
	return ByteHash(bytes, byteSize);
}

VkDescriptorSetLayout VulkanLayoutMgr::GetLayoutHandle(const RHIBindingLayout& bindingLayout) {
	size_t hs = GetLayoutHash(bindingLayout);
	auto iter = m_LayoutMap.find(hs);
	if(iter!= m_LayoutMap.end()) {
		return iter->second;
	}

	VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0 };
	uint32 bindingCount = bindingLayout.Size();
	TempArray<VkDescriptorSetLayoutBinding> bindings(bindingCount);
	for(uint32 i=0; i<bindingCount; ++i) {
		bindings[i].binding = i;
		bindings[i].descriptorType = ToBindingType(bindingLayout[i].Type);
		bindings[i].descriptorCount = bindingLayout[i].Count;
		bindings[i].stageFlags = ToVkShaderStageFlags(bindingLayout[i].StageFlags);
	}
	VkDescriptorSetLayout layoutHandle;
	vkCreateDescriptorSetLayout(m_Device, &info, nullptr, &layoutHandle);
	m_LayoutMap.emplace(hs, layoutHandle);
}

VulkanLayoutMgr::VulkanLayoutMgr(VkDevice device): m_Device(device) {
}

VulkanLayoutMgr::~VulkanLayoutMgr() {
	for(auto& [hs, layoutHandle] : m_LayoutMap) {
		vkDestroyDescriptorSetLayout(m_Device, layoutHandle, nullptr);
	}
}
