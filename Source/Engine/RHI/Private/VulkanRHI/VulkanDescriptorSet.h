#pragma once
#include "VulkanCommon.h"
#include "Core/Public/Container.h"
#include "Core/Public/TVector.h"
#include "RHI/Public/RHIResources.h"

class VulkanDevice;

// a descriptor set handle, with pool
struct DescriptorSetHandle {
	VkDescriptorPool Pool;
	VkDescriptorSet Set;
	bool IsValid() const { return VK_NULL_HANDLE != Set && VK_NULL_HANDLE != Pool; }
	static DescriptorSetHandle InvalidHandle() { return { VK_NULL_HANDLE, VK_NULL_HANDLE }; }
};

// descriptor set manager
class VulkanDescriptorMgr{
public:
	explicit VulkanDescriptorMgr(VulkanDevice* device);
	~VulkanDescriptorMgr();
	VkDescriptorSetLayout GetLayoutHandle(const RHIShaderParemeterLayout& layout);
	DescriptorSetHandle AllocateDS(const RHIShaderParemeterLayout& layout);
	void FreeDS(DescriptorSetHandle handle);
	VkDescriptorPool GetPool();
	void Update();
private:
	VulkanDevice* m_Device;
	TUnorderedMap<size_t, VkDescriptorSetLayout> m_LayoutMap;
	TVector<VkDescriptorPool> m_Pools;
	VkDescriptorPool AddPool();
};

// external resource obj
class VulkanRHIShaderParameterSet : public RHIShaderParameterSet {
public:
	explicit VulkanRHIShaderParameterSet(DescriptorSetHandle handle, VulkanDevice* device);
	~VulkanRHIShaderParameterSet() override;
	void SetUniformBuffer(uint32 binding, RHIBuffer* buffer) override;
	void SetStorageBuffer(uint32 binding, RHIBuffer* buffer) override;
	void SetTexture(uint32 binding, RHITexture* texture) override;
	void SetSampler(uint32 binding, RHISampler* sampler) override;
	VkDescriptorSet GetHandle() const { return m_Handle.Set; }
private:
	DescriptorSetHandle m_Handle;
	VulkanDevice* m_Device;
	
};