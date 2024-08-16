#pragma once
#include "VulkanCommon.h"
#include "Core/Public/Container.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHIResources.h"

class VulkanDevice;

// descriptor set manager
class VulkanDescriptorSetMgr{
public:
	explicit VulkanDescriptorSetMgr(VulkanDevice* device);
	~VulkanDescriptorSetMgr();
	VkDescriptorSetLayout GetLayoutHandle(const RHIShaderParamSetLayout& layout);
	VkDescriptorPool GetPool();
	VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout);
	void FreeDescriptorSet(VkDescriptorSet set);
	void AllocateDescriptorSets(TConstArrayView<VkDescriptorSetLayout> layouts, TArrayView<VkDescriptorSet> sets);
	void FreeDescriptorSets(TArrayView<VkDescriptorSet> sets);
	void Update();
private:
	VulkanDevice* m_Device;
	TUnorderedMap<uint64, VkDescriptorSetLayout> m_LayoutMap;
	TArray<VkDescriptorPool> m_Pools;
	VkDescriptorPool AddPool();
};

// cache descriptors for pipelines, this object must be freed before PSO
class VulkanPipelineDescriptorSetCache {
public:
	struct PipelineLayoutDesc {
		TArray<VkDescriptorSetLayout> LayoutHandles;
		TArray<RHIShaderParamSetLayout> LayoutInfos;
		VkPipelineLayout PipelineLayout;
	};
	NON_COPYABLE(VulkanPipelineDescriptorSetCache);
	NON_MOVEABLE(VulkanPipelineDescriptorSetCache);
	VulkanPipelineDescriptorSetCache(VulkanDevice* device, TConstArrayView<VkDescriptorSetLayout> layouts);
	~VulkanPipelineDescriptorSetCache();
	void SetParameter(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& parameter);
	TConstArrayView<VkDescriptorSet> GetDescriptorSets() const;
private:
	VulkanDevice* m_Device;
	TArray<VkDescriptorSet> m_DescriptorSets;
	TArray<TArray<RHIShaderParam>> m_ParametersCache;
	TConstArrayView<VkDescriptorSetLayout> m_Layouts;// reference, for layout checking.
};

// graphics PSO
class VulkanRHIGraphicsPipelineState : public RHIGraphicsPipelineState {
public:
	explicit VulkanRHIGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, VulkanDevice* device);
	~VulkanRHIGraphicsPipelineState() override;
	void SetName(const char* name) override;
	VkPipeline GetPipelineHandle() const { return m_Pipeline; }
	VkPipelineLayout GetLayoutHandle() const { return m_PipelineLayout; }
	TConstArrayView<VkDescriptorSetLayout> GetSetLayouts() const { return m_SetLayouts; }
private:
	VkPipeline m_Pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout m_PipelineLayout;
	TArray<VkDescriptorSetLayout> m_SetLayouts;
	VulkanDevice* m_Device;
};

// compute PSO
class VulkanRHIComputePipelineState : public RHIComputePipelineState {
public:
	explicit VulkanRHIComputePipelineState(const RHIComputePipelineStateDesc& desc, VulkanDevice* device);
	~VulkanRHIComputePipelineState() override;
	void SetName(const char* name) override;
	VkPipeline GetPipelineHandle() const { return m_Pipeline; }
	VkPipelineLayout GetLayoutHandle() const { return m_PipelineLayout; }
	TConstArrayView<VkDescriptorSetLayout> GetSetLayouts() const { return m_SetLayouts; }
private:
	VkPipeline m_Pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout m_PipelineLayout;
	TArray<VkDescriptorSetLayout> m_SetLayouts;
	VulkanDevice* m_Device;
};

class VulkanPipelineStateContainer {
public:
	NON_COPYABLE(VulkanPipelineStateContainer);
	NON_MOVEABLE(VulkanPipelineStateContainer);
	explicit VulkanPipelineStateContainer(VulkanDevice* device);
	void BindPipelineState(VulkanRHIGraphicsPipelineState* pso);
	void BindPipelineState(VulkanRHIComputePipelineState* pso);
	VulkanPipelineDescriptorSetCache* GetCurrentDescriptorSetCache();
	VulkanRHIComputePipelineState* GetCurrentComputePipelineState();
	VulkanRHIGraphicsPipelineState* GetCurrentGraphicsPipelineState();
	void Reset();
private:
	VulkanDevice* m_Device;
	struct PipelineData {
		VkPipelineBindPoint PipelineBindPoint;
		union {
			VulkanRHIGraphicsPipelineState* GraphicsPipelineState;
			VulkanRHIComputePipelineState* ComputePipelineState;
		};
		TUniquePtr<VulkanPipelineDescriptorSetCache> DescriptorSetCache;
	};
	TArray<PipelineData> m_Pipelines;
};