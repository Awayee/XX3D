#include "VulkanCommand.h"
#include "VulkanResources.h"
#include "VulkanConverter.h"
#include "VulkanDescriptorSet.h"
#include "Core/Public/TArray.h"

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
			XX_ERROR("unsupported layout transition!");
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

void SemaphoreMgr::ResetSmps() {
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

void VulkanCommandMgr::AddGraphicsSubmit(TConstArrayView<VkCommandBuffer> cmds, VkFence fence, bool toPresent) {
	// create a submit info
	uint32 preSmpIdx;
	if(m_SubmitInfos.Empty()) {
		preSmpIdx = INVALID_IDX;
	}
	else {
		SubmitInfo& preInfo = m_SubmitInfos.Back();
		preSmpIdx = preInfo.SignalSmpIdx;
	}
	SubmitInfo& submitInfo = m_SubmitInfos.EmplaceBack();
	submitInfo.CmdStartIdx = m_CmdsToSubmit.Size();
	submitInfo.CmdCount = cmds.Size();
	submitInfo.WaitSmpIdx = preSmpIdx;
	submitInfo.WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;// a graphics cmd must with render pass
	submitInfo.SignalSmpIdx = m_SmpMgr.Allocate();
	submitInfo.ToPresent = toPresent;
	submitInfo.Fence = fence;
	m_CmdsToSubmit.PushBack(cmds.Size(), cmds.Data());
}

void VulkanCommandMgr::FreeCmd(VkCommandBuffer handle) {
	m_CmdsToFree.PushBack(handle);
}

VkSemaphore VulkanCommandMgr::Submit(VkSemaphore presentWaitSmp) {
	if(m_SubmitInfos.Empty()) {
		return VK_NULL_HANDLE;
	}
	uint32 signalSmpIdx = 0;
	for(const SubmitInfo& submitInfo: m_SubmitInfos) {
		VkSubmitInfo vkSubmitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };
		vkSubmitInfo.commandBufferCount = submitInfo.CmdCount;
		vkSubmitInfo.pCommandBuffers = &m_CmdsToSubmit[submitInfo.CmdStartIdx];
		//wait
		TFixedArray<VkSemaphore> waitSmps(2);
		TFixedArray<VkPipelineStageFlags> waitStages(2);
		uint32 waitSmpCount = 0;
		if(INVALID_IDX != submitInfo.WaitSmpIdx) {
			waitSmps[0] = m_SmpMgr.Get(submitInfo.WaitSmpIdx);
			waitStages[0] = submitInfo.WaitStage;
			++waitSmpCount;
		}
		if(submitInfo.ToPresent) {
			waitSmps[1] = presentWaitSmp;
			waitStages[1] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			++waitSmpCount;
		}
		vkSubmitInfo.waitSemaphoreCount = waitSmpCount;
		vkSubmitInfo.pWaitSemaphores = waitSmpCount ? waitSmps.Data() : nullptr;

		// signal
		if(INVALID_IDX != submitInfo.SignalSmpIdx) {
			vkSubmitInfo.signalSemaphoreCount = 1;
			vkSubmitInfo.pSignalSemaphores = &m_SmpMgr.Get(submitInfo.SignalSmpIdx);
			if(submitInfo.SignalSmpIdx > signalSmpIdx) {
				signalSmpIdx = submitInfo.SignalSmpIdx;
			}
		}
		else {
			vkSubmitInfo.signalSemaphoreCount = 0;
			vkSubmitInfo.pSignalSemaphores = nullptr;
		}
		vkQueueSubmit(m_ContextPtr->GraphicsQueue.Handle, 1, &vkSubmitInfo, submitInfo.Fence);
	}

	// free
	VkSemaphore completeSmp = m_SmpMgr.Get(signalSmpIdx);
	m_CmdsToSubmit.Clear();
	m_SubmitInfos.Clear();
	m_SmpMgr.ResetSmps();
	// need to free
	vkFreeCommandBuffers(m_ContextPtr->Device, m_Pool, m_CmdsToFree.Size(), m_CmdsToFree.Data());
	m_CmdsToFree.Clear();

	return completeSmp;
}

RHIVulkanCommandBuffer::RHIVulkanCommandBuffer(VkCommandBuffer handle) : m_Handle(handle), m_IsBegin(false) {
	m_ImageLayoutMgr.Reset(new VulkanImageLayoutMgr());
}

RHIVulkanCommandBuffer::~RHIVulkanCommandBuffer() {
	if (m_IsBegin) {
		vkEndCommandBuffer(m_Handle);
	}
	VulkanCommandMgr::Instance()->FreeCmd(m_Handle);
}

void RHIVulkanCommandBuffer::BeginRenderPass(RHIRenderPass* pass) {
	RHIVulkanRenderPass* vkPass = dynamic_cast<RHIVulkanRenderPass*>(pass);
	vkPass->ResolveImageLayout(m_ImageLayoutMgr.Get());
	VkRenderPass passHandle = vkPass->GetRenderPass();
	VkFramebuffer fbHandle = vkPass->GetFramebuffer();
	auto& desc = pass->GetDesc();
	VkRenderPassBeginInfo info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, nullptr };
	info.renderPass = passHandle;
	info.framebuffer = fbHandle;
	info.renderArea.extent.width = desc.RenderSize.w;
	info.renderArea.extent.height = desc.RenderSize.h;
	info.renderArea.offset.x = 0;
	info.renderArea.offset.y = 0;
	uint32 attachmentCount = desc.ColorTargets.Size() + !!desc.DepthStencilTarget.Target;
	TFixedArray<VkClearValue> clearValues(attachmentCount);
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

	m_CurrentPass = passHandle;
}

void RHIVulkanCommandBuffer::EndRenderPass() {
	vkCmdEndRenderPass(m_Handle);
}

void RHIVulkanCommandBuffer::BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) {
	RHIVulkanGraphicsPipelineState* vkPipeline = dynamic_cast<RHIVulkanGraphicsPipelineState*>(pipeline);
	m_CurrentPipeline = vkPipeline->GetPipelineHandle(m_CurrentPass, m_SubPass);
	m_CurrentPipelineLayout = vkPipeline->GetLayoutHandle();
	m_CurrentPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	vkCmdBindPipeline(m_Handle, m_CurrentPipelineBindPoint, m_CurrentPipeline);
}

void RHIVulkanCommandBuffer::BindComputePipeline(RHIComputePipelineState* pipeline) {
	RHIVulkanComputePipelineState* vkPipeline = dynamic_cast<RHIVulkanComputePipelineState*>(pipeline);
	m_CurrentPipeline = vkPipeline->GetPipelineHandle();
	m_CurrentPipelineLayout = vkPipeline->GetLayoutHandle();
	m_CurrentPipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	vkCmdBindPipeline(m_Handle, m_CurrentPipelineBindPoint, m_CurrentPipeline);
}

void RHIVulkanCommandBuffer::BindShaderParameterSet(uint32 index, RHIShaderParameterSet* set) {
	RHIVulkanShaderParameterSet* vkSet = dynamic_cast<RHIVulkanShaderParameterSet*>(set);
	const VkDescriptorSet dsHandle = vkSet->GetHandle();
	vkCmdBindDescriptorSets(m_Handle, m_CurrentPipelineBindPoint, m_CurrentPipelineLayout, index, 1, &dsHandle, 0, nullptr);
}

void RHIVulkanCommandBuffer::BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) {
	VkBuffer vkBuffer = dynamic_cast<RHIVulkanBufferWithMem*>(buffer)->GetBuffer();
	vkCmdBindVertexBuffers(m_Handle, first, 1, &vkBuffer, &offset);
}

void RHIVulkanCommandBuffer::BindIndexBuffer(RHIBuffer* buffer, uint64 offset) {
	vkCmdBindIndexBuffer(m_Handle, dynamic_cast<RHIVulkanBufferWithMem*>(buffer)->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
}

void RHIVulkanCommandBuffer::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) {
	vkCmdDraw(m_Handle, vertexCount, instanceCount, firstIndex, firstInstance);
}

void RHIVulkanCommandBuffer::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) {
	vkCmdDrawIndexed(m_Handle, indexCount, instanceCount, firstIndex, static_cast<int32>(vertexOffset), firstInstance);
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
	vkCmdCopyBuffer(m_Handle, dynamic_cast<RHIVulkanBufferWithMem*>(srcBuffer)->GetBuffer(), dynamic_cast<RHIVulkanBufferWithMem*>(dstBuffer)->GetBuffer(), 1, &copy);
}

void RHIVulkanCommandBuffer::CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) {
	RHIVulkanTexture* vkTex = dynamic_cast<RHIVulkanTexture*>(texture);
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
	vkCmdCopyBufferToImage(m_Handle, ((RHIVulkanBufferWithMem*)buffer)->GetBuffer(), vkTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void RHIVulkanCommandBuffer::CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region)
{
	VkImageAspectFlags srcAsp = ToImageAspectFlags(srcTex->GetDesc().Flags);
	VkImageAspectFlags dstAsp = ToImageAspectFlags(dstTex->GetDesc().Flags);
	ASSERT(srcAsp == dstAsp, "");
	VkImageCopy copy{};
	copy.srcSubresource.aspectMask = srcAsp;
	copy.srcSubresource.baseArrayLayer = region.SrcSub.BaseLayer;
	copy.srcSubresource.layerCount = region.SrcSub.LayerCount;
	memcpy(&copy.srcOffset, &region.SrcSub.Offset, sizeof(VkOffset3D));
	copy.dstSubresource.aspectMask = dstAsp;
	copy.dstSubresource.baseArrayLayer = region.DstSub.BaseLayer;
	copy.dstSubresource.layerCount = region.DstSub.LayerCount;
	memcpy(&copy.dstOffset, &region.DstSub.Offset, sizeof(VkOffset3D));
	vkCmdCopyImage(m_Handle, ((RHIVulkanTexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ((RHIVulkanTexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
}

void RHIVulkanCommandBuffer::ResourceBarrier(RHITexture* texture, RHITextureSubDesc subDesc, EResourceState stateBefore, EResourceState stateAfter) {
	RHIVulkanTexture* vkTex = dynamic_cast<RHIVulkanTexture*>(texture);
	const VkImageLayout oldLayout = ToImageLayout(stateBefore);
	const VkImageLayout newLayout = ToImageLayout(stateAfter);
	// record
	VulkanImageLayoutWrap* wrap = m_ImageLayoutMgr->GetLayout(texture);
	wrap->CurrentLayout = newLayout;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
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
	RHIVulkanTexture* vkTex = dynamic_cast<RHIVulkanTexture*>(texture);
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
