#include "VulkanCommand.h"
#include "RHIVKResources.h"
#include "VulkanConverter.h"
#include "VulkanUtil.h"
#include "Core/Public/TempArray.h"

VulkanCommandMgr::VulkanCommandMgr(const VulkanContext* context): m_ContextPtr(context), m_Pool(VK_NULL_HANDLE) {
	VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr };
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = m_ContextPtr->GraphicsQueue.FamilyIndex;
	VK_CHECK(vkCreateCommandPool(m_ContextPtr->Device, &commandPoolCreateInfo, nullptr, &m_Pool), "VkCreateCommandPool");
}

VulkanCommandMgr::~VulkanCommandMgr() {
	vkDestroyCommandPool(m_ContextPtr->Device, m_Pool, nullptr);
}

VkCommandBuffer VulkanCommandMgr::NewCmd() {
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr };
	allocateInfo.commandPool = m_Pool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;
	VkCommandBuffer handle;
	if (VK_SUCCESS != vkAllocateCommandBuffers(m_ContextPtr->Device, &allocateInfo, &handle)) {
		return VK_NULL_HANDLE;
	}
	return handle;
}

void VulkanCommandMgr::FreeCmd(VkCommandBuffer handle) {
	m_ToFree.PushBack(handle);
}

void VulkanCommandMgr::Submit(VkCommandBuffer cmd) {
	m_ToSubmit.PushBack(cmd);
}

void VulkanCommandMgr::SubmitInternal() {
	// check deleted command buffers
	VkSubmitInfo info{ VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
	info.waitSemaphoreCount = 0;
	info.pWaitSemaphores = nullptr;
	info.pWaitDstStageMask = nullptr;
	info.commandBufferCount = m_ToSubmit.Size();
	info.pCommandBuffers = m_ToSubmit.Data();
	info.signalSemaphoreCount = 0;
	info.pSignalSemaphores = nullptr;
	vkQueueSubmit(m_ContextPtr->GraphicsQueue.Handle, 1, &info, nullptr);
	m_ToSubmit.Clear();

	vkFreeCommandBuffers(m_ContextPtr->Device, m_Pool, m_ToFree.Size(), m_ToFree.Data());
	m_ToFree.Clear();
}


RHIVulkanCommandBuffer::RHIVulkanCommandBuffer(VkCommandBuffer handle, VulkanCommandMgr* mgr) : m_Handle(handle), m_Mgr(mgr), m_IsBegin(false) {
}

RHIVulkanCommandBuffer::~RHIVulkanCommandBuffer() {
	if (m_IsBegin) {
		vkEndCommandBuffer(m_Handle);
	}
	if(m_Mgr) {
		m_Mgr->FreeCmd(m_Handle);
	}
}

void RHIVulkanCommandBuffer::BeginRenderPass(RHIRenderPass* pass) {
	RHIVulkanRenderPass* vkPass = dynamic_cast<RHIVulkanRenderPass*>(pass);
	VkRenderPassBeginInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr };
	info.renderPass = vkPass->GetRenderPass();
	info.framebuffer = vkPass->GetFramebuffer();
	auto& desc = pass->GetDesc();
	info.renderArea.extent.width = desc.RenderSize.w;
	info.renderArea.extent.height = desc.RenderSize.h;
	info.renderArea.offset.x = 0;
	info.renderArea.offset.y = 0;
	uint32 attachmentCount = desc.ColorTargets.Size() + !!desc.DepthStencilTarget.Target;
	TempArray<VkClearValue> clearValues(attachmentCount);
	uint32 i = 0;
	for (; i < desc.ColorTargets.Size(); ++i) {
		const auto& color = desc.ColorTargets[i].ColorClear;
		memcpy(&clearValues[i].color.float32[0], &color.r, sizeof(color));
	}
	if (desc.DepthStencilTarget.Target) {
		clearValues[i].depthStencil.depth = desc.DepthStencilTarget.DepthClear;
		clearValues[i].depthStencil.stencil = desc.DepthStencilTarget.StencilClear;
		++i;
	}
	info.clearValueCount = attachmentCount;
	info.pClearValues = clearValues.Data();
	vkCmdBeginRenderPass(m_Handle, &info, VK_SUBPASS_CONTENTS_INLINE);

	m_CurrentPass = vkPass->GetRenderPass();
}

void RHIVulkanCommandBuffer::EndRenderPass() {
	vkCmdEndRenderPass(m_Handle);
}

void RHIVulkanCommandBuffer::CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) {
	VkBufferCopy copy{ srcOffset, dstOffset, size };
	vkCmdCopyBuffer(m_Handle, dynamic_cast<RHIVkBuffer*>(srcBuffer)->GetBuffer(), dynamic_cast<RHIVkBuffer*>(dstBuffer)->GetBuffer(), 1, &copy);
}

void RHIVulkanCommandBuffer::CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) {
	RHIVkTexture* vkTex = dynamic_cast<RHIVkTexture*>(texture);
	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = ToImageAspectFlags(texture->GetDesc().Flags);
	region.imageSubresource.mipLevel = mipLevel;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layerCount;
	region.imageOffset = { 0, 0, 0 };
	const USize3D size = vkTex->GetDesc().Size;
	region.imageExtent = { size.w, size.h, size.d };
	vkCmdCopyBufferToImage(m_Handle, ((RHIVkBuffer*)buffer)->GetBuffer(), vkTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void RHIVulkanCommandBuffer::CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region)
{
	VkImageAspectFlags srcAsp = ToImageAspectFlags(srcTex->GetDesc().Flags);
	VkImageAspectFlags dstAsp = ToImageAspectFlags(dstTex->GetDesc().Flags);
	ASSERT(srcAsp == dstAsp, "");
	VkImageBlit blit{};
	blit.srcSubresource.aspectMask = srcAsp;
	blit.srcSubresource.baseArrayLayer = region.srcBaseLayer;
	blit.srcSubresource.layerCount = region.srcLayerCount;
	memcpy(blit.srcOffsets, region.srcOffsets, sizeof(VkOffset3D) * 2);
	blit.dstSubresource.aspectMask = dstAsp;
	blit.dstSubresource.baseArrayLayer = region.dstBaseLayer;
	blit.dstSubresource.layerCount = region.dstLayerCount;
	memcpy(blit.dstOffsets, region.dstOffsets, sizeof(VkOffset3D) * 2);
	vkCmdBlitImage(m_Handle, ((RHIVkTexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ((RHIVkTexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
}

void RHIVulkanCommandBuffer::TransitionTextureLayout(RHITexture* texture, RImageLayout oldLayout, RImageLayout newLayout, uint32 baseMipLevel, uint32 levelCount, uint32 baseLayer, uint32 layerCount, EImageAspectFlags aspect) {
	RHIVkTexture* vkTex = dynamic_cast<RHIVkTexture*>(texture);
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = (VkImageLayout)oldLayout;
	barrier.newLayout = (VkImageLayout)newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkTex->GetImage();
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	barrier.subresourceRange.levelCount = levelCount;
	barrier.subresourceRange.baseArrayLayer = baseLayer;
	barrier.subresourceRange.layerCount = layerCount;
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;
	GetPipelineBarrierStage(barrier.oldLayout, barrier.newLayout, barrier.srcAccessMask, barrier.dstAccessMask, srcStage, dstStage);
	vkCmdPipelineBarrier(m_Handle, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void RHIVulkanCommandBuffer::GenerateMipmap(RHITexture* texture, uint32 levelCount, uint32 baseLayer, uint32 layerCount) {
	RHIVkTexture* vkTex = dynamic_cast<RHIVkTexture*>(texture);
	USize3D size = vkTex->GetDesc().Size;
	const VkImageAspectFlags aspect = ToImageAspectFlags(vkTex->GetDesc().Flags);
	GenerateMipMap(m_Handle, vkTex->GetImage(), levelCount, size.w, size.h, aspect, baseLayer, layerCount);
}

void RHIVulkanCommandBuffer::BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) {
	RHIVulkanGraphicsPipelineState* vkPipeline = dynamic_cast<RHIVulkanGraphicsPipelineState*>(pipeline);
	vkCmdBindPipeline(m_Handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetPipelineHandle(m_CurrentPass, m_SubPass));
}

void RHIVulkanCommandBuffer::BindComputePipeline(RHIComputePipelineState* pipeline) {
	RHIVulkanComputePipelineState* vkPipeline = dynamic_cast<RHIVulkanComputePipelineState*>(pipeline);
	vkCmdBindPipeline(m_Handle, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->GetPipelineHandle());
}

void RHIVulkanCommandBuffer::BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) {
	VkBuffer vkBuffer = dynamic_cast<RHIVkBuffer*>(buffer)->GetBuffer();
	vkCmdBindVertexBuffers(m_Handle, first, 1, &vkBuffer, &offset);
}

void RHIVulkanCommandBuffer::BindIndexBuffer(RHIBuffer* buffer, uint64 offset) {
	vkCmdBindIndexBuffer(m_Handle, dynamic_cast<RHIVkBuffer*>(buffer)->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
}

void RHIVulkanCommandBuffer::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) {
	vkCmdDraw(m_Handle, vertexCount, instanceCount, firstIndex, firstInstance);
}

void RHIVulkanCommandBuffer::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) {
	vkCmdDrawIndexed(m_Handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void RHIVulkanCommandBuffer::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) {
	vkCmdDispatch(m_Handle, groupCountX, groupCountY, groupCountZ);
}

void RHIVulkanCommandBuffer::ClearColorAttachment(const float* color, const IRect& rect) {
	VkClearAttachment clearAttachment;
	clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clearAttachment.clearValue.color.float32[0] = color[0];
	clearAttachment.clearValue.color.float32[1] = color[1];
	clearAttachment.clearValue.color.float32[2] = color[2];
	clearAttachment.clearValue.color.float32[3] = color[3];
	VkClearRect clearRect;
	clearRect.rect.extent.width = static_cast<uint32>(rect.w);
	clearRect.rect.extent.height = static_cast<uint32>(rect.h);
	clearRect.rect.offset.x = rect.x;
	clearRect.rect.offset.y = rect.y;
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	vkCmdClearAttachments(m_Handle, 1, &clearAttachment, 1, &clearRect);
}

void RHIVulkanCommandBuffer::BeginDebugLabel(const char* msg, const float* color) {
	if (nullptr != vkCmdBeginDebugUtilsLabelEXT) {
		VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr };
		labelInfo.pLabelName = msg;
		if (nullptr != color) {
			for (int i = 0; i < 4; ++i) {
				labelInfo.color[i] = color[i];
			}
		}
		vkCmdBeginDebugUtilsLabelEXT(m_Handle, &labelInfo);
	}
}

void RHIVulkanCommandBuffer::EndDebugLabel() {
	if (nullptr != vkCmdEndDebugUtilsLabelEXT) {
		vkCmdEndDebugUtilsLabelEXT(m_Handle);
	}
}

void RHIVulkanCommandBuffer::CmdBegin() {
	VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
	info.flags = 0;
	info.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(m_Handle, &info);
}
