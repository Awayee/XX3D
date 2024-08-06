#include "VulkanDescriptorSet.h"
#include "Core/Public/Algorithm.h"
#include "VulkanConverter.h"
#include "Core/Public/Log.h"
#include "VulkanResources.h"
#include "VulkanDevice.h"

inline size_t GetLayoutHash(const RHIShaderParemeterLayout& bindingLayout) {
	size_t byteSize = sizeof(bindingLayout[0]) * bindingLayout.Size();
	const uint8* bytes = reinterpret_cast<const uint8*>(bindingLayout.Data());
	return ByteHash(bytes, byteSize);
}

VkDescriptorSetLayout VulkanDescriptorMgr::GetLayoutHandle(const RHIShaderParemeterLayout& layout) {
	size_t hs = GetLayoutHash(layout);
	auto iter = m_LayoutMap.find(hs);
	if(iter!= m_LayoutMap.end()) {
		return iter->second;
	}

	VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0 };
	uint32 bindingCount = layout.Size();
	TFixedArray<VkDescriptorSetLayoutBinding> bindings(bindingCount);
	for(uint32 i=0; i<bindingCount; ++i) {
		bindings[i].binding = i;
		bindings[i].descriptorType = ToVkDescriptorType(layout[i].Type);
		bindings[i].descriptorCount = layout[i].Count;
		bindings[i].stageFlags = ToVkShaderStageFlags(layout[i].StageFlags);
	}
	VkDescriptorSetLayout layoutHandle;
	vkCreateDescriptorSetLayout(m_Device->GetDevice(), &info, nullptr, &layoutHandle);
	m_LayoutMap.emplace(hs, layoutHandle);
	return layoutHandle;
}

DescriptorSetHandle VulkanDescriptorMgr::AllocateDS(const RHIShaderParemeterLayout& layout) {
	VkDescriptorSetLayout layoutHandle = GetLayoutHandle(layout);
	VkDescriptorSet handle = nullptr;
	for (uint32 i = 0; i < m_Pools.Size(); ++i) {
		VkDescriptorSetAllocateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, m_Pools[i], 1, &layoutHandle };
		if (VK_ERROR_OUT_OF_POOL_MEMORY != vkAllocateDescriptorSets(m_Device->GetDevice(), &info, &handle)) {
			return DescriptorSetHandle{ m_Pools[i], handle };
		}
	}
	return DescriptorSetHandle::InvalidHandle();
}

void VulkanDescriptorMgr::FreeDS(DescriptorSetHandle handle) {
	vkFreeDescriptorSets(m_Device->GetDevice(), handle.Pool, 1, &handle.Set);
}

VkDescriptorPool VulkanDescriptorMgr::GetPool() {
	return m_Pools[0];
}

void VulkanDescriptorMgr::Update() {
}

VkDescriptorPool VulkanDescriptorMgr::AddPool() {
	VkDescriptorPoolCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr};
	info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	constexpr uint8 bindingCount = static_cast<uint8>(EBindingType::MaxNum);
	static const uint32 s_CountPerType[bindingCount] = {8, 128, 64, 128, 16, 128};
	VkDescriptorPoolSize sizes[bindingCount];
	uint32 allCount = 0;
	for(uint8 i=0; i<static_cast<uint8>(EBindingType::MaxNum); ++i) {
		sizes[i].type = ToVkDescriptorType(static_cast<EBindingType>(i));
		sizes[i].descriptorCount = s_CountPerType[i];
		allCount += sizes[i].descriptorCount;
	}
	info.poolSizeCount = bindingCount;
	info.pPoolSizes = sizes;
	info.maxSets = allCount;
	VkDescriptorPool handle;
	vkCreateDescriptorPool(m_Device->GetDevice(), &info, nullptr, &handle);
	m_Pools.PushBack(handle);
	return handle;
}

VulkanDescriptorMgr::VulkanDescriptorMgr(VulkanDevice* device) : m_Device(device) {
	AddPool();
}

VulkanDescriptorMgr::~VulkanDescriptorMgr() {
	for (auto& [hs, layoutHandle] : m_LayoutMap) {
		vkDestroyDescriptorSetLayout(m_Device->GetDevice(), layoutHandle, nullptr);
	}
	for (VkDescriptorPool& pool : m_Pools) {
		vkDestroyDescriptorPool(m_Device->GetDevice(), pool, nullptr);
		pool = VK_NULL_HANDLE;
	}
}

VulkanRHIShaderParameterSet::VulkanRHIShaderParameterSet(DescriptorSetHandle handle, VulkanDevice* device): m_Handle(handle),  m_Device(device) {
}

VulkanRHIShaderParameterSet::~VulkanRHIShaderParameterSet() {
	m_Device->GetDescriptorMgr()->FreeDS(m_Handle);
}

void VulkanRHIShaderParameterSet::SetUniformBuffer(uint32 binding, RHIBuffer* buffer) {
	// TODO use large buffer
	VulkanRHIBuffer* vkBuffer = dynamic_cast<VulkanRHIBuffer*>(buffer);
	VkDescriptorBufferInfo bufferInfo{ vkBuffer->GetBuffer(), 0, vkBuffer->GetDesc().ByteSize };
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
		m_Handle.Set, binding,
		0,
		1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		nullptr,
		&bufferInfo,
		nullptr
	};
	vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &write, 0, nullptr);
}

void VulkanRHIShaderParameterSet::SetStorageBuffer(uint32 binding, RHIBuffer* buffer) {
	VulkanRHIBuffer* vkBuffer = dynamic_cast<VulkanRHIBuffer*>(buffer);
	VkDescriptorBufferInfo bufferInfo{ vkBuffer->GetBuffer(), 0, vkBuffer->GetDesc().ByteSize };
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
		m_Handle.Set, binding,
		0,
		1,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		nullptr,
		&bufferInfo,
		nullptr
	};
	vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &write, 0, nullptr);
}

void VulkanRHIShaderParameterSet::SetTexture(uint32 binding, RHITexture* texture) {
	VulkanRHITexture* vkTexture = dynamic_cast<VulkanRHITexture*>(texture);
	VkDescriptorImageInfo imageInfo{ VK_NULL_HANDLE, vkTexture->GetDefaultView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
	m_Handle.Set, binding,
	0,
	1,
	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	&imageInfo,
	nullptr,
	nullptr
	};
	vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &write, 0, nullptr);
}

void VulkanRHIShaderParameterSet::SetSampler(uint32 binding, RHISampler* sampler) {
	VulkanRHISampler *vkSampler = dynamic_cast<VulkanRHISampler*>(sampler);
	VkDescriptorImageInfo imageInfo{ vkSampler->GetSampler(), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
	m_Handle.Set, binding,
	0,
	1,
	VK_DESCRIPTOR_TYPE_SAMPLER,
	&imageInfo,
	nullptr,
	nullptr
	};
	vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &write, 0, nullptr);
}
