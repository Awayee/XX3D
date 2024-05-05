#pragma once
#include "VulkanCommon.h"
#include "Core/Public/Container.h"
#include "Core/Public/TVector.h"
#include "RHI/Public/RHIResources.h"
#include "Core/Public/TSingleton.h"

// a descriptor set handle, with pool
struct DescriptorSetHandle {
	VkDescriptorPool Pool;
	VkDescriptorSet Set;
	bool IsValid() const { return VK_NULL_HANDLE != Set && VK_NULL_HANDLE != Pool; }
	static DescriptorSetHandle InvalidHandle() { return { VK_NULL_HANDLE, VK_NULL_HANDLE }; }
};

// descriptor set manager
class VulkanDSMgr: public TSingleton<VulkanDSMgr>{
public:
	VkDescriptorSetLayout GetLayoutHandle(const RHIShaderParemeterLayout& layout);
	DescriptorSetHandle AllocateDS(const RHIShaderParemeterLayout& layout);
	void FreeDS(DescriptorSetHandle handle);
	VkDevice GetDevice() const { return m_Device; }
	VkDescriptorPool GetPool();
	void Update();
private:
	friend TSingleton<VulkanDSMgr>;
	VkDevice m_Device;
	TUnorderedMap<size_t, VkDescriptorSetLayout> m_LayoutMap;
	TVector<VkDescriptorPool> m_Pools;
	explicit VulkanDSMgr(VkDevice device);
	~VulkanDSMgr();
	VkDescriptorPool AddPool();
};

// external resource obj
class RHIVulkanShaderParameterSet : public RHIShaderParameterSet {
public:
	explicit RHIVulkanShaderParameterSet(DescriptorSetHandle handle);
	~RHIVulkanShaderParameterSet() override;
	void SetUniformBuffer(uint32 binding, RHIBuffer* buffer) override;
	void SetStorageBuffer(uint32 binding, RHIBuffer* buffer) override;
	void SetTexture(uint32 binding, RHITexture* texture) override;
	void SetSampler(uint32 binding, RHISampler* sampler) override;
	VkDescriptorSet GetHandle() const { return m_Handle.Set; }
private:
	DescriptorSetHandle m_Handle;
};