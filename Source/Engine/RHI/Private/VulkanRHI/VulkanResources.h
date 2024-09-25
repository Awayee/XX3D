#pragma once
#include "RHI/Public/RHIResources.h"
#include "VulkanMemory.h"
#include "Core/Public/String.h"

class VulkanRHI;
class VulkanDevice;

class VulkanBuffer: public RHIBuffer {
public:
	VulkanBuffer(const RHIBufferDesc& desc, VulkanDevice* device);
	~VulkanBuffer() override;
	void SetName(const char* name) override;
	void UpdateData(const void* data, uint32 byteSize, uint32 offset) override;
	VkBuffer GetBuffer() const { return m_Allocation.GetBuffer(); }
protected:
	VulkanDevice* m_Device;
	BufferAllocation m_Allocation;
};

class VulkanBufferImpl: public VulkanBuffer{
public:
	using VulkanBuffer::VulkanBuffer;
	void UpdateData(const void* data, uint32 byteSize, uint32 offset) override;
};

class VulkanRHITexture : public RHITexture {
public:
	VulkanRHITexture(const RHITextureDesc& desc) : RHITexture(desc) {}
	virtual VkImageView GetView(RHITextureSubRes subRes) = 0;
	virtual VkImage GetImage() = 0;
	VkImageView GetDefaultView();
};

class VulkanTextureImpl: public VulkanRHITexture {
public:
	VulkanTextureImpl(const RHITextureDesc& desc, VulkanDevice* device, VkImage image, ImageAllocation&& allocation);
	~VulkanTextureImpl() override;
	VkImage GetImage() override { return m_Image; }
	VkImageView GetView(RHITextureSubRes subRes) override;
	void SetName(const char* name) override;
	void UpdateData(const void* data, uint32 byteSize, RHITextureSubRes subRes, IOffset3D offset) override;
protected:
	VulkanDevice* m_Device;
	VkImage m_Image;
	ImageAllocation m_Allocation;
	struct SubResView {
		RHITextureSubRes SubRes;
		VkImageView View;
	};
	// views are sorted by array layer
	TArray<TArray<SubResView>> m_Views;
	// m_Image will be destroy in this object if m_IsImageOwner is true
	VkImageView CreateView(RHITextureSubRes subRes);
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
	VulkanRHIShader(EShaderStageFlags type, const char* code, uint32 codeSize, const XString& entryFunc, VulkanDevice* device);
	~VulkanRHIShader() override;
	void SetName(const char* name) override;
	VkPipelineShaderStageCreateInfo GetShaderStageInfo() const;
private:
	XString m_EntryName;
	VkShaderModule m_ShaderModule;
	VulkanDevice* m_Device;
};
