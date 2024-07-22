#include "VulkanCommand.h"
#include "VulkanResources.h"
#include "VulkanConverter.h"
#include "VulkanDescriptorSet.h"
#include "Core/Public/TArray.h"
#include "VulkanDevice.h"

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
			LOG_ERROR("unsupported layout transition!");
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
		VK_ASSERT(vkCreateSemaphore(m_Device, &info, nullptr, &handle), "vkCreateSemaphore");
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

VulkanCommandMgr::VulkanCommandMgr(VulkanDevice* device): m_Device(device), m_Pool(VK_NULL_HANDLE), m_UploadCmd(VK_NULL_HANDLE) {
	VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr };
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = m_Device->GetGraphicsQueue().FamilyIndex;
	VkDevice vkDevice = m_Device->GetDevice();
	VK_ASSERT(vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &m_Pool), "VkCreateCommandPool");
	m_SmpMgr.Initialize(vkDevice, 512);
}

VulkanCommandMgr::~VulkanCommandMgr() {
	vkDestroyCommandPool(m_Device->GetDevice(), m_Pool, nullptr);
}

RHICommandBufferPtr VulkanCommandMgr::NewCmd() {
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr };
	allocateInfo.commandPool = m_Pool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;
	VkCommandBuffer handle;
	if (VK_SUCCESS != vkAllocateCommandBuffers(m_Device->GetDevice(), &allocateInfo, &handle)) {
		return nullptr;
	}
	return new VulkanRHICommandBuffer(m_Device, handle);
}

RHICommandBuffer* VulkanCommandMgr::GetUploadCmd() {
	if(!m_UploadCmd) {
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr };
		allocateInfo.commandPool = m_Pool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VkCommandBuffer handle;
		if (VK_SUCCESS != vkAllocateCommandBuffers(m_Device->GetDevice(), &allocateInfo, &handle)) {
			return nullptr;
		}
		m_UploadCmd = new VulkanRHICommandBuffer(m_Device, handle);
	}
	return m_UploadCmd.Get();
}

void VulkanCommandMgr::AddGraphicsSubmit(TConstArrayView<VkCommandBuffer> cmds, VkFence fence, bool toPresent) {
	// create a submit info
	uint32 preSmpIdx;
	if(m_SubmitInfos.Empty()) {
		preSmpIdx = VK_INVALID_IDX;
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
	// uploading commands must be executed before graphics commands 
	if(m_UploadCmd) {
		VkCommandBuffer handle = ((VulkanRHICommandBuffer*)m_UploadCmd.Get())->GetHandle();
		AddGraphicsSubmit({ handle }, VK_NULL_HANDLE, false);
	}

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
		if(VK_INVALID_IDX != submitInfo.WaitSmpIdx) {
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
		if(VK_INVALID_IDX != submitInfo.SignalSmpIdx) {
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
		vkQueueSubmit(m_Device->GetGraphicsQueue().Handle, 1, &vkSubmitInfo, submitInfo.Fence);
	}

	// free
	VkSemaphore completeSmp = m_SmpMgr.Get(signalSmpIdx);
	m_CmdsToSubmit.Clear();
	m_SubmitInfos.Clear();
	m_SmpMgr.ResetSmps();
	// need to free
	vkFreeCommandBuffers(m_Device->GetDevice(), m_Pool, m_CmdsToFree.Size(), m_CmdsToFree.Data());
	m_CmdsToFree.Clear();

	return completeSmp;
}

VulkanRHICommandBuffer::VulkanRHICommandBuffer(VulkanDevice* device, VkCommandBuffer handle) : m_Device(device), m_Handle(handle) {
	m_ImageLayoutMgr.Reset(new VulkanImageLayoutMgr());
	CmdBegin();
}

VulkanRHICommandBuffer::~VulkanRHICommandBuffer() {
	ASSERT(!m_IsBegin, "Command buffer must be Destroyed after ending.");
	m_Device->GetCommandMgr()->FreeCmd(m_Handle);
}

void VulkanRHICommandBuffer::BeginRendering(const RHIRenderPassDesc& desc) {
	VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO, nullptr, 0 };
	auto& area = desc.RenderArea;
	renderingInfo.renderArea = { {area.x, area.y}, {area.w, area.h} };
	renderingInfo.layerCount = 1;
	// resolve color attachments
	const uint32 colorAttachmentCount = desc.ColorTargets.Size();
	TFixedArray<VkRenderingAttachmentInfo> colorAttachments(colorAttachmentCount);
	for(uint32 i=0; i<colorAttachmentCount; ++i) {
		colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachments[i].pNext = nullptr;
		const auto& target = desc.ColorTargets[i];
		colorAttachments[i].imageView = ((VulkanRHITexture*)target.Target)->GetView();
		colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		colorAttachments[i].loadOp = ToVkAttachmentLoadOp(target.LoadOp);
		colorAttachments[i].storeOp = ToVkAttachmentStoreOp(target.StoreOp);
		auto clear = target.ColorClear;
		colorAttachments[i].clearValue.color.float32[0] = clear.r;
		colorAttachments[i].clearValue.color.float32[1] = clear.g;
		colorAttachments[i].clearValue.color.float32[2] = clear.b;
		colorAttachments[i].clearValue.color.float32[3] = clear.a;
	}
	renderingInfo.colorAttachmentCount = desc.ColorTargets.Size();
	renderingInfo.pColorAttachments = colorAttachments.Data();

	// resolve depth attachments
	VkRenderingAttachmentInfo dsAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, nullptr };
	auto& depthStencilTarget = desc.DepthStencilTarget;
	if(depthStencilTarget.Target) {
		dsAttachment.imageView = ((VulkanRHITexture*)depthStencilTarget.Target)->GetView();
		dsAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		dsAttachment.loadOp = ToVkAttachmentLoadOp(depthStencilTarget.DepthLoadOp);
		dsAttachment.storeOp = ToVkAttachmentStoreOp(depthStencilTarget.DepthStoreOp);
		dsAttachment.clearValue.depthStencil.depth = depthStencilTarget.DepthClear;
		dsAttachment.clearValue.depthStencil.stencil = depthStencilTarget.StencilClear;
		renderingInfo.pDepthAttachment = &dsAttachment;
		renderingInfo.pStencilAttachment = &dsAttachment;
	}

	vkCmdBeginRendering(m_Handle, &renderingInfo);
}

void VulkanRHICommandBuffer::EndRendering() {
	vkCmdEndRendering(m_Handle);
}

void VulkanRHICommandBuffer::BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) {
	VulkanRHIGraphicsPipelineState* vkPipeline = static_cast<VulkanRHIGraphicsPipelineState*>(pipeline);
	m_CurrentPipeline = vkPipeline->GetPipelineHandle();
	m_CurrentPipelineLayout = vkPipeline->GetLayoutHandle();
	m_CurrentPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	vkCmdBindPipeline(m_Handle, m_CurrentPipelineBindPoint, m_CurrentPipeline);
}

void VulkanRHICommandBuffer::BindComputePipeline(RHIComputePipelineState* pipeline) {
	VulkanRHIComputePipelineState* vkPipeline = static_cast<VulkanRHIComputePipelineState*>(pipeline);
	m_CurrentPipeline = vkPipeline->GetPipelineHandle();
	m_CurrentPipelineLayout = vkPipeline->GetLayoutHandle();
	m_CurrentPipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	vkCmdBindPipeline(m_Handle, m_CurrentPipelineBindPoint, m_CurrentPipeline);
}

void VulkanRHICommandBuffer::BindShaderParameterSet(uint32 index, RHIShaderParameterSet* set) {
	VulkanRHIShaderParameterSet* vkSet = static_cast<VulkanRHIShaderParameterSet*>(set);
	const VkDescriptorSet dsHandle = vkSet->GetHandle();
	vkCmdBindDescriptorSets(m_Handle, m_CurrentPipelineBindPoint, m_CurrentPipelineLayout, index, 1, &dsHandle, 0, nullptr);
}

void VulkanRHICommandBuffer::BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) {
	VkBuffer vkBuffer = static_cast<VulkanRHIBuffer*>(buffer)->GetBuffer();
	vkCmdBindVertexBuffers(m_Handle, first, 1, &vkBuffer, &offset);
}

void VulkanRHICommandBuffer::BindIndexBuffer(RHIBuffer* buffer, uint64 offset) {
	vkCmdBindIndexBuffer(m_Handle, static_cast<VulkanRHIBuffer*>(buffer)->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
}

void VulkanRHICommandBuffer::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) {
	vkCmdDraw(m_Handle, vertexCount, instanceCount, firstIndex, firstInstance);
}

void VulkanRHICommandBuffer::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) {
	vkCmdDrawIndexed(m_Handle, indexCount, instanceCount, firstIndex, static_cast<int32>(vertexOffset), firstInstance);
}

void VulkanRHICommandBuffer::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) {
	vkCmdDispatch(m_Handle, groupCountX, groupCountY, groupCountZ);
}

void VulkanRHICommandBuffer::ClearColorAttachment(const float* color, const IRect& rect) {
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

void VulkanRHICommandBuffer::CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) {
	VkBufferCopy copy{ srcOffset, dstOffset, size };
	vkCmdCopyBuffer(m_Handle, static_cast<VulkanRHIBuffer*>(srcBuffer)->GetBuffer(), static_cast<VulkanRHIBuffer*>(dstBuffer)->GetBuffer(), 1, &copy);
}

void VulkanRHICommandBuffer::CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) {
	VulkanRHITexture* vkTex = static_cast<VulkanRHITexture*>(texture);
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
	vkCmdCopyBufferToImage(m_Handle, ((VulkanRHIBuffer*)buffer)->GetBuffer(), vkTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void VulkanRHICommandBuffer::CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region)
{
	VkImageAspectFlags srcAsp = ToImageAspectFlags(srcTex->GetDesc().Flags);
	VkImageAspectFlags dstAsp = ToImageAspectFlags(dstTex->GetDesc().Flags);
	ASSERT(srcAsp == dstAsp, "");
	VkImageCopy copy{};
	copy.srcSubresource.aspectMask = srcAsp;
	copy.srcSubresource.baseArrayLayer = region.SrcSub.ArrayLayer;
	copy.srcSubresource.layerCount = 1;
	memcpy(&copy.srcOffset, &region.SrcSub.Offset, sizeof(VkOffset3D));
	copy.dstSubresource.aspectMask = dstAsp;
	copy.dstSubresource.baseArrayLayer = region.DstSub.ArrayLayer;
	copy.dstSubresource.layerCount = 1;
	memcpy(&copy.dstOffset, &region.DstSub.Offset, sizeof(VkOffset3D));
	vkCmdCopyImage(m_Handle, ((VulkanRHITexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ((VulkanRHITexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
}

void VulkanRHICommandBuffer::PipelineBarrier(RHITexture* texture, RHITextureSubDesc subDesc, EResourceState stateBefore, EResourceState stateAfter) {
	VulkanRHITexture* vkTex = static_cast<VulkanRHITexture*>(texture);
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
	barrier.subresourceRange.layerCount = subDesc.LayerCount;
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;
	GetPipelineBarrierStage(barrier.oldLayout, barrier.newLayout, barrier.srcAccessMask, barrier.dstAccessMask, srcStage, dstStage);
	vkCmdPipelineBarrier(m_Handle, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanRHICommandBuffer::GenerateMipmap(RHITexture* texture, uint32 levelCount, uint32 baseLayer, uint32 layerCount) {
	VulkanRHITexture* vkTex = static_cast<VulkanRHITexture*>(texture);
	USize3D size = vkTex->GetDesc().Size;
	const VkImageAspectFlags aspect = ToImageAspectFlags(vkTex->GetDesc().Flags);
	GenerateMipMap(m_Handle, vkTex->GetImage(), levelCount, size.w, size.h, aspect, baseLayer, layerCount);
}

void VulkanRHICommandBuffer::BeginDebugLabel(const char* msg, const float* color) {
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

void VulkanRHICommandBuffer::EndDebugLabel() {
	if (nullptr != vkCmdEndDebugUtilsLabelEXT) {
		vkCmdEndDebugUtilsLabelEXT(m_Handle);
	}
}

void VulkanRHICommandBuffer::CmdBegin() {
	VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
	info.flags = 0;
	info.pInheritanceInfo = nullptr;
	vkBeginCommandBuffer(m_Handle, &info);
}
