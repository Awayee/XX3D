#include "VulkanCommand.h"
#include "VulkanResources.h"
#include "VulkanConverter.h"
#include "Core/Public/TempArray.h"

namespace {
	void GetPipelineBarrierStage(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags& srcAccessMask, VkAccessFlags& dstAccessMask, VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage) {
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			srcAccessMask = 0;
			dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		// for getGuidAndDepthOfMouseClickOnRenderSceneForUI() get depthimage
		else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		// for generating mipmapped image
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else
		{
			ERROR("unsupported layout transition!");
		}
	}


	void GenerateMipMap(VkCommandBuffer cmd, VkImage image, uint32 levelCount, uint32 width, uint32 height, VkImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount) {
		for (uint32 i = 1; i < levelCount; i++)
		{
			VkImageBlit imageBlit{};
			imageBlit.srcSubresource.aspectMask = aspect;
			imageBlit.srcSubresource.baseArrayLayer = baseLayer;
			imageBlit.srcSubresource.layerCount = layerCount;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcOffsets[1].x = std::max((int32_t)(width >> (i - 1)), 1);
			imageBlit.srcOffsets[1].y = std::max((int32_t)(height >> (i - 1)), 1);
			imageBlit.srcOffsets[1].z = 1;

			imageBlit.dstSubresource.aspectMask = aspect;
			imageBlit.dstSubresource.layerCount = layerCount;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstOffsets[1].x = std::max((int32_t)(width >> i), 1);
			imageBlit.dstOffsets[1].y = std::max((int32_t)(height >> i), 1);
			imageBlit.dstOffsets[1].z = 1;

			VkImageSubresourceRange mipSubRange{};
			mipSubRange.aspectMask = aspect;
			mipSubRange.baseMipLevel = i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = layerCount;

			VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange = mipSubRange;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier);

			vkCmdBlitImage(cmd,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageBlit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier);
		}

		VkImageSubresourceRange mipSubRange{};
		mipSubRange.aspectMask = aspect;
		mipSubRange.baseMipLevel = 0;
		mipSubRange.levelCount = levelCount;
		mipSubRange.baseArrayLayer = baseLayer;
		mipSubRange.layerCount = layerCount;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange = mipSubRange;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier);
	}
}

SemaphoreMgr::~SemaphoreMgr() {
	Clear();
}

void SemaphoreMgr::Initialize(VkDevice device, uint32 maxSize) {
	m_Device = device;
}

uint32 SemaphoreMgr::Allocate() {
	if(m_CurIndex >= m_Semaphores.Size()) {
		VkSemaphore handle;
		VkSemaphoreCreateInfo info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
		VK_CHECK(vkCreateSemaphore(m_Device, &info, nullptr, &handle), "vkCreateSemaphore");
		m_Semaphores.PushBack(handle);
	}
	return m_CurIndex++;
}

VkSemaphore& SemaphoreMgr::Get(uint32 idx) {
	ASSERT(idx < m_Semaphores.Size(), "");
	return m_Semaphores[idx];
}

void SemaphoreMgr::Reset() {
	m_CurIndex = 0;
}

void SemaphoreMgr::Clear() {
	for (VkSemaphore smp : m_Semaphores) {
		vkDestroySemaphore(m_Device, smp, nullptr);
	}
	m_Semaphores.Clear();
	m_CurIndex = 0;
}

VulkanCommandMgr::VulkanCommandMgr(const VulkanContext* context): m_ContextPtr(context), m_Pool(VK_NULL_HANDLE) {
	VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr };
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = m_ContextPtr->GraphicsQueue.FamilyIndex;
	VK_CHECK(vkCreateCommandPool(m_ContextPtr->Device, &commandPoolCreateInfo, nullptr, &m_Pool), "VkCreateCommandPool");
	m_SmpMgr.Initialize(m_ContextPtr->Device, 512);
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
	m_CmdsToFree.PushBack(handle);
}

void VulkanCommandMgr::SubmitGraphicsCommand(uint32 count, const VkCommandBuffer* handles, VkFence fence) {
	// create a submit info
	uint32 preSmpIdx;
	if(m_SubmitInfos.Empty()) {
		preSmpIdx = INVALID_IDX;
	}
	else {
		SubmitInfo& preInfo = m_SubmitInfos.Back();
		preSmpIdx = m_SmpMgr.Allocate();
		preInfo.SignalSmpIdx = preSmpIdx;
	}
	SubmitInfo& submitInfo = m_SubmitInfos.EmplaceBack();
	submitInfo.CmdStartIdx = m_CmdsToSubmit.Size();
	submitInfo.CmdCount = count;
	submitInfo.WaitSmpIdx = preSmpIdx;
	submitInfo.SignalSmpIdx = INVALID_IDX;
	submitInfo.Fence = fence;
	m_CmdsToSubmit.PushBack(count, handles);
}

void VulkanCommandMgr::SubmitGraphicsCommand(VkCommandBuffer cmd, VkFence fence) {
	SubmitGraphicsCommand(1, &cmd, fence);
}

void VulkanCommandMgr::Update() {
	TempArray<VkSubmitInfo> infos(m_SubmitInfos.Size());
	uint32 submitIdx = 0;
	VkFence tempFence = VK_NULL_HANDLE;
	for (uint32 i = 0; i < m_SubmitInfos.Size(); ++i) {
		const SubmitInfo& submitInfo = m_SubmitInfos[i];
		if(0 == i) {
			tempFence = submitInfo.Fence;
		}
		// check whether fence is same, or submit
		if(tempFence != submitInfo.Fence) {
			vkQueueSubmit(m_ContextPtr->GraphicsQueue.Handle, submitIdx, infos.Data(), tempFence);
			submitIdx = 0;
			tempFence = submitInfo.Fence;
		}
		VkSubmitInfo& vkSubmitInfo = infos[submitIdx++];
		vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		vkSubmitInfo.pNext = nullptr;
		// wait smp
		if(INVALID_IDX != submitInfo.WaitSmpIdx) {
			vkSubmitInfo.waitSemaphoreCount = 1;
			vkSubmitInfo.pWaitSemaphores = &m_SmpMgr.Get(submitInfo.WaitSmpIdx);
		}
		else {
			vkSubmitInfo.waitSemaphoreCount = 0;
			vkSubmitInfo.pWaitSemaphores = nullptr;
		}

		// signal smp
		if(INVALID_IDX != submitInfo.SignalSmpIdx) {
			vkSubmitInfo.signalSemaphoreCount = 1;
			vkSubmitInfo.pSignalSemaphores = &m_SmpMgr.Get(submitInfo.SignalSmpIdx);
		}
		else {
			vkSubmitInfo.signalSemaphoreCount = 0;
			vkSubmitInfo.pSignalSemaphores = nullptr;
		}
		vkSubmitInfo.commandBufferCount = submitInfo.CmdCount;
		vkSubmitInfo.pCommandBuffers = &m_CmdsToSubmit[submitInfo.CmdStartIdx];
	}
	// the last batch
	vkQueueSubmit(m_ContextPtr->GraphicsQueue.Handle, submitIdx, infos.Data(), tempFence);

	vkFreeCommandBuffers(m_ContextPtr->Device, m_Pool, m_CmdsToFree.Size(), m_CmdsToFree.Data());
	m_CmdsToFree.Clear();
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

void RHIVulkanCommandBuffer::ResourceBarrier(RHITexture* texture, RHITextureSubDesc subDesc, EResourceState stateBefore, EResourceState stateAfter) {
	RHIVkTexture* vkTex = dynamic_cast<RHIVkTexture*>(texture);
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = ToImageLayout(stateBefore);
	barrier.newLayout = ToImageLayout(stateAfter);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkTex->GetImage();
	barrier.subresourceRange.aspectMask = ToImageAspectFlags(vkTex->GetDesc().Flags);
	barrier.subresourceRange.baseMipLevel = subDesc.BaseMip;
	barrier.subresourceRange.levelCount = subDesc.NumMips;
	barrier.subresourceRange.baseArrayLayer = subDesc.BaseLayer;
	barrier.subresourceRange.layerCount = subDesc.NumLayers;
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
