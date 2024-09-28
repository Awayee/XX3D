#pragma once
#include "VulkanCommon.h"
#include "Core/Public/Container.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHIResources.h"

class VulkanDevice;
struct VulkanPipelineLayout;
class VulkanDynamicBufferAllocator;
// descriptor set manager
class VulkanDescriptorSetMgr{
public:
	explicit VulkanDescriptorSetMgr(VulkanDevice* device);
	~VulkanDescriptorSetMgr();
	VkDescriptorSetLayout GetLayoutHandle(const RHIShaderParamSetLayout& layout);
	VkDescriptorPool GetPool();
	VkDescriptorPool GetReservePool();
	VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout);
	bool AllocateDescriptorSets(TConstArrayView<VkDescriptorSetLayout> layouts, TArrayView<VkDescriptorSet> sets);
	void BeginFrame();
private:
	VulkanDevice* m_Device;
	TUnorderedMap<uint64, VkDescriptorSetLayout> m_LayoutMap;
	TArray<VkDescriptorPool> m_Pools;
	VkDescriptorPool m_ReservePool; // for imgui
	uint32 m_PoolMaxIndex;
	void CreatePool(VkDescriptorPool* poolPtr);
	void AddPool();
};

// cache parameters for writing descriptor set
class VulkanDescriptorSetParamCache {
public:
	NON_COPYABLE(VulkanDescriptorSetParamCache);
	VulkanDescriptorSetParamCache(const RHIShaderParamSetLayout& layout);
	VulkanDescriptorSetParamCache(VulkanDescriptorSetParamCache&&)noexcept = default;
	void SetParam(const VulkanDynamicBufferAllocator* allocator, uint32 bindIndex, const RHIShaderParam& param);
	TArray<VkWriteDescriptorSet>& GetWrites() { return m_Writes; }
private:
	const RHIShaderParamSetLayout& m_LayoutRef;
	TArray<VkDescriptorBufferInfo> m_WriteBuffers;
	TArray<VkDescriptorImageInfo> m_WriteImages;
	TArray<VkWriteDescriptorSet> m_Writes;
};

// cache descriptors for pipelines, this object must be freed before PSO
class VulkanPipelineDescriptorSetCache {
public:
	NON_COPYABLE(VulkanPipelineDescriptorSetCache);
	NON_MOVEABLE(VulkanPipelineDescriptorSetCache);
	VulkanPipelineDescriptorSetCache(VulkanDevice* device, const VulkanPipelineLayout* layout);
	~VulkanPipelineDescriptorSetCache();
	void SetParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& param);
	void Bind(VkCommandBuffer cmd, VkPipelineBindPoint point);
private:
	VulkanDevice* m_Device;
	const VulkanPipelineLayout* m_Layout;// reference, for layout checking.
	TArray<VulkanDescriptorSetParamCache> m_ParamCaches;
	TArray<VkDescriptorSet> m_BindingSets; // current binding descriptor sets
	TArray<bool> m_DirtySets; // mark set as dirty, to create new descriptor set handle
	TArray<VkDescriptorSet> m_AllDescriptorSets;// all allocated descriptor sets
	VkDescriptorSet ReallocateSet(uint32 setIndex);
};

// pipeline layout owned by pso
struct VulkanPipelineLayout {
	VkPipelineLayout Handle{ VK_NULL_HANDLE };
	TArray<VkDescriptorSetLayout> DescriptorSetLayouts{ VK_NULL_HANDLE };
	TConstArrayView<RHIShaderParamSetLayout> PipelineLayoutMeta;
	void Build(VulkanDevice* device, TConstArrayView<RHIShaderParamSetLayout> meta);
};

// graphics PSO
class VulkanRHIGraphicsPipelineState : public RHIGraphicsPipelineState {
public:
	explicit VulkanRHIGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, VulkanDevice* device);
	~VulkanRHIGraphicsPipelineState() override;
	void SetName(const char* name) override;
	VkPipeline GetPipelineHandle() const { return m_Pipeline; }
	const VulkanPipelineLayout* GetPipelineLayout()const { return &m_PipelineLayout; }
private:
	VkPipeline m_Pipeline{ VK_NULL_HANDLE };
	VulkanPipelineLayout m_PipelineLayout;
	VulkanDevice* m_Device;
};

// compute PSO
class VulkanRHIComputePipelineState : public RHIComputePipelineState {
public:
	explicit VulkanRHIComputePipelineState(const RHIComputePipelineStateDesc& desc, VulkanDevice* device);
	~VulkanRHIComputePipelineState() override;
	void SetName(const char* name) override;
	VkPipeline GetPipelineHandle() const { return m_Pipeline; }
	const VulkanPipelineLayout* GetPipelineLayout()const { return &m_PipelineLayout; }
private:
	VkPipeline m_Pipeline{ VK_NULL_HANDLE };
	VulkanPipelineLayout m_PipelineLayout;
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