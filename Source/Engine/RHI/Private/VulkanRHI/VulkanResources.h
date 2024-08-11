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
	void UpdateData(const void* data, uint32 byteSize, uint32 offset) override;
	VkBuffer GetBuffer() const { return m_Allocation.GetBuffer(); }
private:
	friend VulkanRHI;
	VulkanDevice* m_Device;
	BufferAllocation m_Allocation;
};

class VulkanRHITexture: public RHITexture {
public:
	VulkanRHITexture(const RHITextureDesc& desc, VulkanDevice* device, VkImage image, ImageAllocation&& allocation);
	~VulkanRHITexture() override;
	VkImage GetImage() const { return m_Image; }
	virtual VkImageView GetView(ETextureSRVType type, RHITextureSubDesc sub);
	VkImageView GetDefaultView();
	VkImageView Get2DView(uint8 mipIndex, uint32 arrayIndex);
	VkImageView GetCubeView(uint8 mipIndex, uint32 arrayIndex);// only for cube map
	void SetName(const char* name) override;
	void UpdateData(uint32 byteSize, const void* data, RHITextureOffset offset) override;
protected:
	VulkanDevice* m_Device;
	VkImage m_Image;
	ImageAllocation m_Allocation;
	TVector<VkImageView> m_AllViews;
	// view indices for search
	uint32 m_DefaultViewIndex;
	TVector<uint32> m_2DViewIndices;
	TVector<uint32> m_CubeViewIndices;
	// m_Image will be destroy in this object if m_IsImageOwner is true
	bool m_IsImageOwner;
	uint32 CreateView(VkImageViewType type, VkImageAspectFlags aspectFlags, uint8 mipIndex, uint8 mipCount, uint32 arrayIndex, uint32 arrayCount);
};

// Depth stencil textures use special views.
class VulkanDepthStencilTexture: public VulkanRHITexture {
public:
	VulkanDepthStencilTexture(const RHITextureDesc& desc, VulkanDevice* device, VkImage image, ImageAllocation&& allocation);
	VkImageView GetView(ETextureSRVType type, RHITextureSubDesc sub) override;
	VkImageView GetDepthView();
	VkImageView GetStencilView();
	VkImageView GetDepthStencilView();
private:
	uint32 m_DepthViewIndex;
	uint32 m_StencilViewIndex;
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
	VulkanDevice* m_Device;
	VkFence m_Handle{VK_NULL_HANDLE};
};

class VulkanRHIShader: public RHIShader {
public:
	VulkanRHIShader(EShaderStageFlagBit type, const char* code, size_t codeSize, const XString& entryFunc, VulkanDevice* device);
	~VulkanRHIShader() override;
	void SetName(const char* name) override;
	XX_NODISCARD VkPipelineShaderStageCreateInfo GetShaderStageInfo() const;
private:
	XString m_EntryName;
	VkShaderModule m_ShaderModule;
	VulkanDevice* m_Device;
};
