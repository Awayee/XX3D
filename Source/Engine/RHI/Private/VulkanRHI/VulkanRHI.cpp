#include "Math/Public/MathBase.h"
#include "VulkanRHI.h"
#include "VulkanConverter.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "VulkanCommand.h"
#include "VulkanPipeline.h"
#include "VulkanViewport.h"

static constexpr uint32 MIN_API_VERSION{ VK_VERSION_1_2 };

VulkanRHI::VulkanRHI(WindowHandle wnd, USize2D extent, const RHIInitConfig& cfg) {

	// initialize context
	m_Context.Reset(new VulkanContext(cfg.EnableDebug, MIN_API_VERSION));
	// Pick GPU
	TArray<const char*> extensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkPhysicalDevice physicalDevice = m_Context->PickPhysicalDevice();
	ASSERT(physicalDevice != VK_NULL_HANDLE, "Failed to pick GPU!");
	// Create logic device
	m_Device.Reset(new VulkanDevice(m_Context.Get(), physicalDevice));
	// Create viewport
	m_Viewport.Reset(new VulkanViewport(m_Context.Get(), m_Device.Get(), wnd, extent, cfg));

	LOG_INFO("RHI: Vulkan initialized successfully!");
}

VulkanRHI::~VulkanRHI() {
	m_Viewport.Reset();
	m_Device.Reset();
}

void VulkanRHI::BeginFrame() {
	m_Device->GetCommandContext()->BeginFrame();
	m_Device->GetUploader()->BeginFrame();
	m_Device->GetDescriptorMgr()->BeginFrame();
}

void VulkanRHI::BeginRendering() {
	m_Device->GetDynamicBufferAllocator()->GC();
	m_Device->GetDynamicBufferAllocator()->UnmapAllocations();
}

ERHIFormat VulkanRHI::GetDepthFormat() {
	switch (m_Device->GetFormats().DepthFormat) {
	case(VK_FORMAT_D32_SFLOAT):
		return ERHIFormat::D32_SFLOAT;
	case(VK_FORMAT_D24_UNORM_S8_UINT):
		return ERHIFormat::D24_UNORM_S8_UINT;
	case(VK_FORMAT_D16_UNORM_S8_UINT):
		return ERHIFormat::D16_UNORM;
	default:
		return ERHIFormat::Undefined;
	}
}

RHIViewport* VulkanRHI::GetViewport() {
	return m_Viewport.Get();
}

const VulkanContext* VulkanRHI::GetContext() {
	return m_Context.Get();
}

VulkanDevice* VulkanRHI::GetDevice() {
	return m_Device.Get();
}

RHIBufferPtr VulkanRHI::CreateBuffer(const RHIBufferDesc& desc) {
	return RHIBufferPtr(new VulkanBufferImpl(desc, m_Device.Get()));
}

RHITexturePtr VulkanRHI::CreateTexture(const RHITextureDesc& desc) {
	return RHITexturePtr(new VulkanTextureImpl(desc, GetDevice()));
}

RHISamplerPtr VulkanRHI::CreateSampler(const RHISamplerDesc& desc) {
	VkSamplerCreateInfo info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, nullptr, 0 };
	info.minFilter = ToFilter(desc.Filter);
	info.magFilter = info.minFilter;
	info.addressModeU = ToAddressMode(desc.AddressU);
	info.addressModeV = ToAddressMode(desc.AddressV);
	info.addressModeW = ToAddressMode(desc.AddressW);
	info.mipmapMode = desc.Filter == ESamplerFilter::Trilinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
	info.mipLodBias = desc.LODBias;
	info.minLod = desc.MinLOD;
	info.maxLod = desc.MaxLOD;
	if(desc.Filter == ESamplerFilter::AnisotropicPoint || desc.Filter == ESamplerFilter::AnisotropicLinear) {
		info.maxAnisotropy = Math::FMin(desc.MaxAnisotropy, m_Device->GetProperties().limits.maxSamplerAnisotropy);
	}
	info.anisotropyEnable = static_cast<VkBool32>(info.maxAnisotropy > 1.0f);
	info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	VkSampler samplerHandle;
	if (VK_SUCCESS != vkCreateSampler(m_Device->GetDevice(), &info, nullptr, &samplerHandle)) {
		return nullptr;
	}
	return RHISamplerPtr(new VulkanRHISampler(desc, GetDevice(), samplerHandle));
}

RHIFencePtr VulkanRHI::CreateFence(bool sig) {
	VkFenceCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.pNext = nullptr;
	info.flags = sig ? VK_FENCE_CREATE_SIGNALED_BIT : 0; // the fence is initialized as signaled
	VkFence fence;
	if(VK_SUCCESS == vkCreateFence(m_Device->GetDevice(), &info, nullptr, &fence)) {
		return RHIFencePtr(new VulkanRHIFence(fence, GetDevice()));
	}
	return nullptr;
}

RHIShaderPtr VulkanRHI::CreateShader(EShaderStageFlags type, const char* codeData, uint32 codeSize, const XString& entryFunc) {
	return RHIShaderPtr(new VulkanRHIShader(type, codeData, codeSize, entryFunc, GetDevice()));
}

RHIGraphicsPipelineStatePtr VulkanRHI::CreateGraphicsPipelineState(const RHIGraphicsPipelineStateDesc& desc) {
	return RHIGraphicsPipelineStatePtr(new VulkanRHIGraphicsPipelineState(desc, GetDevice()));
}

RHIComputePipelineStatePtr VulkanRHI::CreateComputePipelineState(const RHIComputePipelineStateDesc& desc) {
	return RHIComputePipelineStatePtr(new VulkanRHIComputePipelineState(desc, GetDevice()));
}

RHICommandBufferPtr VulkanRHI::CreateCommandBuffer(EQueueType queue) {
	return m_Device->GetCommandContext()->AllocateCommandBuffer(queue);
}

void VulkanRHI::SubmitCommandBuffers(TArrayView<RHICommandBuffer*> cmds, EQueueType queue, RHIFence* fence, bool bPresent) {
	TArrayView<VulkanCommandBuffer*> vulkanCmds((VulkanCommandBuffer**)(cmds.Data()), cmds.Size());
	VkFence fenceHandle = fence ? ((VulkanRHIFence*)fence)->GetFence() : VK_NULL_HANDLE;
	VkSemaphore smp = bPresent ? m_Viewport->GetCurrentSemaphore() : VK_NULL_HANDLE;
	m_Device->GetCommandContext()->SubmitCommandBuffers(vulkanCmds, queue, smp, fenceHandle);
}

RHIDynamicBuffer VulkanRHI::AllocateDynamicBuffer(EBufferFlags bufferFlags, uint32 bufferSize, const void* bufferData, uint32 stride) {
	auto a = m_Device->GetDynamicBufferAllocator()->Allocate(ToBufferUsage(bufferFlags), bufferSize, bufferData);
	return RHIDynamicBuffer{ a.BufferIndex, a.Offset, a.Size, stride};
}
