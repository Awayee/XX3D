#pragma once
#include "VulkanCommon.h"
#include "Core/Public/Container.h"
#include "RHI/Public/RHIResources.h"

class VulkanDevice;
struct VulkanPipelineLayout;
class VulkanDynamicBufferAllocator;
// descriptor set manager

struct VulkanDescriptorInfo {
	uint32 Binding : 5;
	uint32 NumElements : 15;
	EBindingType Type : 4;
	EShaderStageFlags StageFlags : 8;
	VulkanDescriptorInfo(RHIShaderBinding binding, EShaderStageFlags stageFlags);
};
class VulkanDescriptorSetMgr{
public:
	explicit VulkanDescriptorSetMgr(VulkanDevice* device);
	~VulkanDescriptorSetMgr();
	VkDescriptorSetLayout GetLayoutHandle(TConstArrayView<VulkanDescriptorInfo> layout);
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

// pipeline layout owned by pso
struct VulkanPipelineLayout {
	VkPipelineLayout Handle{ VK_NULL_HANDLE };
	TArray<VkDescriptorSetLayout> DescriptorSetLayouts;
	TArray<RHIShaderBinding> BindingsMeta;
	TStaticArray<uint8, RHIShaderBinding::MAX_SET> BindingOffsets;
	void Build(VulkanDevice* device, RHIShaderBindingSet& bindingSet);
	RHIShaderBinding GetBindingMeta(uint32 set, uint32 binding);
	TConstArrayView<RHIShaderBinding> GetBindings(uint32 set) const;
};

// graphics PSO
class VulkanRHIGraphicsPipelineState : public RHIGraphicsPipelineState {
public:
	explicit VulkanRHIGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, VulkanDevice* device);
	~VulkanRHIGraphicsPipelineState() override;
	void SetNameInternal(const char* name) override;
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
	explicit VulkanRHIComputePipelineState(RHIShader* shader, VulkanDevice* device);
	~VulkanRHIComputePipelineState() override;
	void SetNameInternal(const char* name) override;
	VkPipeline GetPipelineHandle() const { return m_Pipeline; }
	const VulkanPipelineLayout* GetPipelineLayout()const { return &m_PipelineLayout; }
private:
	VkPipeline m_Pipeline{ VK_NULL_HANDLE };
	VulkanPipelineLayout m_PipelineLayout;
	VulkanDevice* m_Device;
};

// cache parameters for writing descriptor set
class VulkanDescriptorSetParamCache {
public:
	NON_COPYABLE(VulkanDescriptorSetParamCache);
	VulkanDescriptorSetParamCache(TConstArrayView<RHIShaderBinding> bindings);
	VulkanDescriptorSetParamCache(VulkanDescriptorSetParamCache&&)noexcept = default;
	// Cache descriptor write info, if info is modified, return true
	bool SetParam(const VulkanDynamicBufferAllocator* allocator, uint32 bindIndex, const RHIShaderParam& param);
	TArray<VkWriteDescriptorSet>& GetWrites() { return m_Writes; }
private:
	TConstArrayView<RHIShaderBinding> m_BindingsRef;
	TArray<VkDescriptorBufferInfo> m_WriteBuffers;
	TArray<VkDescriptorImageInfo> m_WriteImages;
	TArray<VkWriteDescriptorSet> m_Writes;
};

class VulkanPipelineDescriptorSetCache {
public:
	NON_COPYABLE(VulkanPipelineDescriptorSetCache);
	NON_MOVEABLE(VulkanPipelineDescriptorSetCache);
	VulkanPipelineDescriptorSetCache(VulkanDevice* device);
	~VulkanPipelineDescriptorSetCache()  =default;
	void BindGraphicsPipeline(const VulkanRHIGraphicsPipelineState* pipeline);
	void BindComputePipeline(const VulkanRHIComputePipelineState* pipeline);
	void SetParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& param);
	void Bind(VkCommandBuffer cmd);
	void Reset();
	VkPipelineBindPoint GetVkPipelineBindPoint();
	VkPipeline GetVkPipeline();
private:
	VulkanDevice* m_Device;
	union {
		const VulkanRHIGraphicsPipelineState* m_GraphicsPipeline;
		const VulkanRHIComputePipelineState* m_ComputePipeline;
		void* m_PipelineData;
	};
	EPipelineType m_PipelineType;
	TArray<VulkanDescriptorSetParamCache> m_ParamCaches;
	TArray<VkDescriptorSet> m_BindingSets; // current binding descriptor sets
	TArray<bool> m_DirtySets; // mark set as dirty, to create new descriptor set handle
	TArray<VkDescriptorSet> m_AllDescriptorSets;// all allocated descriptor sets
	void SetupParameterLayout();
	VkDescriptorSet ReallocateSet(uint32 setIndex);
	const VulkanPipelineLayout* GetLayout();
};