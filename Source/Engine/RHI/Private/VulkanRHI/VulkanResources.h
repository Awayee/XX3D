#pragma once
#include "RHI/Public/RHIResources.h"
#include "VulkanMemory.h"
#include "Core/Public/String.h"

class VulkanRHI;

class VulkanDevice;

class VulkanRHIResource: public RHIResource {
protected:
	VkDevice m_DeviceHandle{VK_NULL_HANDLE};
};

class VulkanRHIBuffer: public RHIBuffer{
public:
	VulkanRHIBuffer(const RHIBufferDesc& desc, VulkanDevice* device, BufferAllocation&& alloc);
	~VulkanRHIBuffer()override;
	void SetName(const char* name) override;
	void UpdateData(const void* data, uint32 byteSize) override;
	VkBuffer GetBuffer() const { return m_Allocation.GetBuffer(); }
private:
	friend VulkanRHI;
	VulkanDevice* m_Device;
	BufferAllocation m_Allocation;
};

class VulkanRHITexture: public RHITexture {
public:
	VulkanRHITexture(const RHITextureDesc& desc, VulkanDevice* device, VkImage image, VkImageView view, ImageAllocation&& allocation);
	~VulkanRHITexture() override;
	VkImage GetImage() const { return m_Image; }
	VkImageView GetView() const { return m_View; }
	void SetName(const char* name) override;
	void UpdateData(RHITextureOffset offset, uint32 byteSize, const void* data) override;
protected:
	VulkanDevice* m_Device;
	VkImage m_Image;
	VkImageView m_View;
	ImageAllocation m_Allocation;
};

class VulkanRHISampler: public RHISampler{
private:
	friend VulkanRHI;
	VulkanDevice* m_Device;
	VkSampler m_Sampler;
public:
	VulkanRHISampler(const RHISamplerDesc& desc, VulkanDevice* device, VkSampler sampler);
	~VulkanRHISampler()override;
	VkSampler GetSampler() { return m_Sampler; }
	void SetName(const char* name) override;
};

class VulkanRHIFence : public RHIFence {
public:
	explicit VulkanRHIFence(VkFence fence, VulkanDevice* device);
	~VulkanRHIFence() override;
	void Wait() override;
	void Reset() override;
	void SetName(const char* name) override;
	VkFence GetFence() const { return m_Handle; }
private:
	static constexpr uint32 WAIT_MAX = 0xffff;
	VulkanDevice* m_Device;
	VkFence m_Handle{VK_NULL_HANDLE};
};

class VulkanRHIShader: public RHIShader {
public:
	VulkanRHIShader(EShaderStageFlagBit type, const char* code, size_t codeSize, const char* funcName, VulkanDevice* device);
	~VulkanRHIShader() override;
	void SetName(const char* name) override;
	XX_NODISCARD VkPipelineShaderStageCreateInfo GetShaderStageInfo() const;
private:
	XXString m_EntryName;
	VkShaderModule m_ShaderModule;
	VulkanDevice* m_Device;
};

class VulkanRHIGraphicsPipelineState : public RHIGraphicsPipelineState {
public:
	explicit VulkanRHIGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc, VulkanDevice* device);
	~VulkanRHIGraphicsPipelineState() override;
	void SetName(const char* name) override;
	VkPipeline GetPipelineHandle() const { return m_Pipeline; }
	VkPipelineLayout GetLayoutHandle() const { return m_PipelineLayout; }
private:
	friend VulkanRHI;
	XXString m_Name;
	VkPipelineLayout m_PipelineLayout{VK_NULL_HANDLE};
	VkPipeline m_Pipeline{VK_NULL_HANDLE};
	VulkanDevice* m_Device;
};

class VulkanRHIComputePipelineState: public RHIComputePipelineState {
public:
	explicit VulkanRHIComputePipelineState(const RHIComputePipelineStateDesc& desc, VulkanDevice* device);
	~VulkanRHIComputePipelineState() override;
	void SetName(const char* name) override;
	VkPipeline GetPipelineHandle() const { return m_Pipeline; }
	VkPipelineLayout GetLayoutHandle() const { return m_PipelineLayout; }
private:
	friend VulkanRHI;
	VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };
	VkPipeline m_Pipeline{ VK_NULL_HANDLE };
	VulkanDevice* m_Device;
};

// layout manager in one pass
struct VulkanImageLayoutWrap {
	VkImageLayout InitialLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	VkImageLayout CurrentLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	VkImageLayout FinalLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
};
class VulkanImageLayoutMgr {
public:
	VulkanImageLayoutMgr() = default;
	~VulkanImageLayoutMgr() = default;
	VulkanImageLayoutWrap* GetLayout(RHITexture* tex);
	const VulkanImageLayoutWrap* GetLayout(RHITexture* tex) const;
private:
	std::unordered_map<VkImage, VulkanImageLayoutWrap> m_ImageLayoutMap;
};

class VulkanRHIRenderPass: public RHIRenderPass {
public:
	explicit VulkanRHIRenderPass(const RHIRenderPassDesc& desc, VulkanDevice* device);
	~VulkanRHIRenderPass() override;
	void SetName(const char* name) override;
	void ResolveImageLayout(const VulkanImageLayoutMgr* layoutMgr);
	VkRenderPass GetRenderPass() const { return m_RenderPass; }
	uint32 GetSubPass() const { return m_SubPass; }
	VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
private:
	VkRenderPass m_RenderPass;
	uint32 m_SubPass;
	VkFramebuffer m_Framebuffer;
	const VulkanImageLayoutMgr* m_ImageLayoutMgr;
	VulkanDevice* m_Device;
	void DestroyHandle();
	void CreateHandle();
};
