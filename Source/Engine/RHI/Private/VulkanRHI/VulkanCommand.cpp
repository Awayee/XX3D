#include "VulkanCommand.h"
#include "VulkanResources.h"
#include "VulkanConverter.h"
#include "VulkanPipeline.h"
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
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			srcAccessMask = 0;
			dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		// for get depth
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
		// for swapchain back buffer rendering
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
			srcAccessMask = 0;
			dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		// for shader resource
		else if(oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dstStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		}
		else if(oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dstStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		}
		// for swapchain present.
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
			srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dstAccessMask = 0;
			srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
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

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandContext* context, VkCommandBuffer handle, VkSemaphore smp) :
m_Owner(context),
m_Handle(handle),
m_Semaphore(smp),
// TODO stage mask would be reset
m_StageMask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT){
	m_PipelineStateContainer.Reset(new VulkanPipelineStateContainer(context->GetDevice()));
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
	ASSERT(!m_HasBegun, "Command buffer must be Destroyed after ending.");
	m_Owner->FreeCommandBuffer(this);
}

void VulkanCommandBuffer::Reset() {
	vkResetCommandBuffer(m_Handle, 0);
	m_PipelineStateContainer->Reset();
	m_ScissorDirty = true;
	m_ViewportDirty = true;
	CheckBegin();
}

void VulkanCommandBuffer::BeginRendering(const RHIRenderPassInfo& info) {
	VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO, nullptr, 0 };
	auto& area = info.RenderArea;
	renderingInfo.renderArea = { {area.x, area.y}, {area.w, area.h} };
	renderingInfo.layerCount = 1;
	// resolve color attachments
	const uint32 colorAttachmentCount = info.GetNumColorTargets();
	TFixedArray<VkRenderingAttachmentInfo> colorAttachments(colorAttachmentCount);
	for(uint32 i=0; i<colorAttachmentCount; ++i) {
		colorAttachments[i] = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO , nullptr };
		const auto& target = info.ColorTargets[i];
		colorAttachments[i].imageView = ((VulkanRHITexture*)target.Target)->Get2DView(target.MipIndex, target.LayerIndex);
		colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachments[i].resolveMode = VK_RESOLVE_MODE_NONE;
		colorAttachments[i].resolveImageView = VK_NULL_HANDLE;
		colorAttachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachments[i].loadOp = ToVkAttachmentLoadOp(target.LoadOp);
		colorAttachments[i].storeOp = ToVkAttachmentStoreOp(target.StoreOp);
		auto clear = target.ColorClear;
		colorAttachments[i].clearValue.color.float32[0] = clear.r;
		colorAttachments[i].clearValue.color.float32[1] = clear.g;
		colorAttachments[i].clearValue.color.float32[2] = clear.b;
		colorAttachments[i].clearValue.color.float32[3] = clear.a;
	}
	renderingInfo.colorAttachmentCount = colorAttachmentCount;
	renderingInfo.pColorAttachments = colorAttachments.Data();

	// resolve depth attachments
	VkRenderingAttachmentInfo dsAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, nullptr };
	auto& depthStencilTarget = info.DepthStencilTarget;
	if(depthStencilTarget.Target) {
		dsAttachment.imageView = ((VulkanRHITexture*)depthStencilTarget.Target)->GetDefaultView();
		dsAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		dsAttachment.loadOp = ToVkAttachmentLoadOp(depthStencilTarget.DepthLoadOp);
		dsAttachment.storeOp = ToVkAttachmentStoreOp(depthStencilTarget.DepthStoreOp);
		dsAttachment.clearValue.depthStencil.depth = depthStencilTarget.DepthClear;
		dsAttachment.clearValue.depthStencil.stencil = depthStencilTarget.StencilClear;
		renderingInfo.pDepthAttachment = &dsAttachment;
		renderingInfo.pStencilAttachment = &dsAttachment;
	}

	vkCmdBeginRendering(m_Handle, &renderingInfo);
	m_Viewport = { (float)area.x, (float)area.y, (float)area.w, (float)area.h, 0.0f, 1.0f };
	m_Scissor = { {area.x, area.y}, {area.w, area.h} };
}

void VulkanCommandBuffer::EndRendering() {
	vkCmdEndRendering(m_Handle);
}

void VulkanCommandBuffer::BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) {
	VulkanRHIGraphicsPipelineState* vkPipeline = static_cast<VulkanRHIGraphicsPipelineState*>(pipeline);
	vkCmdBindPipeline(m_Handle, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetPipelineHandle());
	m_PipelineStateContainer->BindPipelineState(vkPipeline);
}

void VulkanCommandBuffer::BindComputePipeline(RHIComputePipelineState* pipeline) {
	VulkanRHIComputePipelineState* vkPipeline = static_cast<VulkanRHIComputePipelineState*>(pipeline);
	vkCmdBindPipeline(m_Handle, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline->GetPipelineHandle());
	m_PipelineStateContainer->BindPipelineState(vkPipeline);
}

void VulkanCommandBuffer::SetShaderParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& parameter) {
	if(VulkanPipelineDescriptorSetCache* dsCache = m_PipelineStateContainer->GetCurrentDescriptorSetCache()) {
		dsCache->SetParam(setIndex, bindIndex, parameter);
	}
}

void VulkanCommandBuffer::BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset) {
	VkBuffer vkBuffer = static_cast<VulkanRHIBuffer*>(buffer)->GetBuffer();
	vkCmdBindVertexBuffers(m_Handle, first, 1, &vkBuffer, &offset);
}

void VulkanCommandBuffer::BindIndexBuffer(RHIBuffer* buffer, uint64 offset) {
	vkCmdBindIndexBuffer(m_Handle, static_cast<VulkanRHIBuffer*>(buffer)->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
}

void VulkanCommandBuffer::SetViewport(FRect rect, float minDepth, float maxDepth) {
	m_Viewport = { rect.x, rect.y, rect.w, rect.h, minDepth, maxDepth };
	m_ViewportDirty = true;
}

void VulkanCommandBuffer::SetScissor(Rect rect) {
	m_Scissor = { {rect.x, rect.y}, {rect.w, rect.h} };
	m_ScissorDirty = true;
}

void VulkanCommandBuffer::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) {
	PrepareDraw();
	vkCmdDraw(m_Handle, vertexCount, instanceCount, firstIndex, firstInstance);
}

void VulkanCommandBuffer::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) {
	PrepareDraw();
	vkCmdDrawIndexed(m_Handle, indexCount, instanceCount, firstIndex, static_cast<int32>(vertexOffset), firstInstance);
}

void VulkanCommandBuffer::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) {
	PrepareDraw();
	vkCmdDispatch(m_Handle, groupCountX, groupCountY, groupCountZ);
}

void VulkanCommandBuffer::ClearColorTarget(uint32 targetIndex, const float* color, const IRect& rect) {
	VkClearAttachment clearAttachment;
	clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clearAttachment.colorAttachment = targetIndex;
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

void VulkanCommandBuffer::CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) {
	VkBufferCopy copy{ srcOffset, dstOffset, size };
	vkCmdCopyBuffer(m_Handle, static_cast<VulkanRHIBuffer*>(srcBuffer)->GetBuffer(), static_cast<VulkanRHIBuffer*>(dstBuffer)->GetBuffer(), 1, &copy);
}

void VulkanCommandBuffer::CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount) {
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
	const auto& desc = vkTex->GetDesc();
	region.imageExtent = { desc.Width, desc.Height, desc.Depth };
	vkCmdCopyBufferToImage(m_Handle, ((VulkanRHIBuffer*)buffer)->GetBuffer(), vkTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void VulkanCommandBuffer::CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region)
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

void VulkanCommandBuffer::TransitionTextureState(RHITexture* texture, EResourceState stateBefore, EResourceState stateAfter, RHITextureSubDesc subDesc) {
	VulkanRHITexture* vkTex = static_cast<VulkanRHITexture*>(texture);
	const VkImageLayout oldLayout = ToImageLayout(stateBefore);
	const VkImageLayout newLayout = ToImageLayout(stateAfter);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkTex->GetImage();
	barrier.subresourceRange.aspectMask = ToImageAspectFlags(vkTex->GetDesc().Flags);
	barrier.subresourceRange.baseMipLevel = subDesc.MipIndex;
	barrier.subresourceRange.levelCount = subDesc.MipCount;
	barrier.subresourceRange.baseArrayLayer = subDesc.LayerIndex;
	barrier.subresourceRange.layerCount = subDesc.LayerCount;
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;
	GetPipelineBarrierStage(barrier.oldLayout, barrier.newLayout, barrier.srcAccessMask, barrier.dstAccessMask, srcStage, dstStage);
	vkCmdPipelineBarrier(m_Handle, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanCommandBuffer::GenerateMipmap(RHITexture* texture, uint32 levelCount, uint32 baseLayer, uint32 layerCount) {
	VulkanRHITexture* vkTex = static_cast<VulkanRHITexture*>(texture);
	const auto& desc = vkTex->GetDesc();
	const VkImageAspectFlags aspect = ToImageAspectFlags(vkTex->GetDesc().Flags);
	GenerateMipMap(m_Handle, vkTex->GetImage(), levelCount, desc.Width, desc.Height, aspect, baseLayer, layerCount);
}

void VulkanCommandBuffer::BeginDebugLabel(const char* msg, const float* color) {
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

void VulkanCommandBuffer::EndDebugLabel() {
	if (nullptr != vkCmdEndDebugUtilsLabelEXT) {
		vkCmdEndDebugUtilsLabelEXT(m_Handle);
	}
}

void VulkanCommandBuffer::CheckBegin() {
	if(!m_HasBegun) {
		VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
		info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		info.pInheritanceInfo = nullptr;
		vkBeginCommandBuffer(m_Handle, &info);
		m_HasBegun = true;
	}
}

void VulkanCommandBuffer::CheckEnd() {
	if(m_HasBegun) {
		vkEndCommandBuffer(m_Handle);
		m_HasBegun = false;
	}
}

void VulkanCommandBuffer::PrepareDraw() {
	if(VulkanPipelineDescriptorSetCache* dsCache = m_PipelineStateContainer->GetCurrentDescriptorSetCache()) {
		if(VulkanRHIGraphicsPipelineState* graphicsPSO = m_PipelineStateContainer->GetCurrentGraphicsPipelineState()) {
			dsCache->Bind(m_Handle, VK_PIPELINE_BIND_POINT_GRAPHICS);
			if(m_ViewportDirty) {
				vkCmdSetViewport(m_Handle, 0, 1, &m_Viewport);
				m_ViewportDirty = false;
			}
			if(m_ScissorDirty) {
				vkCmdSetScissor(m_Handle, 0, 1, &m_Scissor);
				m_ScissorDirty = false;
			}
		}
		else if(VulkanRHIComputePipelineState* computePSO = m_PipelineStateContainer->GetCurrentComputePipelineState()) {
			dsCache->Bind(m_Handle, VK_PIPELINE_BIND_POINT_COMPUTE);
		}
	}
}

VulkanCommandContext::VulkanCommandContext(VulkanDevice* device) :m_Device(device), m_CommandPool(VK_NULL_HANDLE) {
	VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr };
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = m_Device->GetGraphicsQueue().FamilyIndex;
	VkDevice vkDevice = m_Device->GetDevice();
	VK_ASSERT(vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &m_CommandPool), "VkCreateCommandPool");
	m_UploadCmd = AllocateCommandBuffer();
}

VulkanCommandContext::~VulkanCommandContext() {
	m_UploadCmd.Reset();// cmds must be freed before command pool destroyed.
	vkDestroyCommandPool(m_Device->GetDevice(), m_CommandPool, nullptr);
}

RHICommandBufferPtr VulkanCommandContext::AllocateCommandBuffer() {
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr };
	allocateInfo.commandPool = m_CommandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;
	VkCommandBuffer handle;
	VK_CHECK(vkAllocateCommandBuffers(m_Device->GetDevice(), &allocateInfo, &handle));
	VkSemaphore smp;
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
	vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &smp);
	return RHICommandBufferPtr(new VulkanCommandBuffer(this, handle, smp));
}

void VulkanCommandContext::FreeCommandBuffer(VulkanCommandBuffer* cmd) {
	vkFreeCommandBuffers(m_Device->GetDevice(), m_CommandPool, 1, &cmd->m_Handle);
	vkDestroySemaphore(m_Device->GetDevice(), cmd->m_Semaphore, nullptr);
}

void VulkanCommandContext::SubmitCommandBuffer(VulkanCommandBuffer* cmd, VkSemaphore waitSemaphore, VkFence fence) {
	SubmitCommandBuffers({ cmd }, waitSemaphore, fence);
}

void VulkanCommandContext::SubmitCommandBuffers(TArrayView<VulkanCommandBuffer*> cmds, VkSemaphore waitSemaphore, VkFence fence) {
	VkSubmitInfo info{ VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr };

	// collect the wait semaphores
	auto& lastSubmission = GetLastSubmission();
	TArray<VkSemaphore> waitSemaphores; waitSemaphores.Reserve(lastSubmission.Size() + 1);
	TArray<VkPipelineStageFlags> waitStageMasks; waitStageMasks.Reserve(lastSubmission.Size() + 1);
	for(auto* cmd: lastSubmission) {
		if(cmd->m_Semaphore) {
			waitSemaphores.PushBack(cmd->m_Semaphore);
			waitStageMasks.PushBack(cmd->m_StageMask);
		}
	}
	// add the outer semaphore if need.
	if(waitSemaphore) {
		waitSemaphores.PushBack(waitSemaphore);
		waitStageMasks.PushBack(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	info.waitSemaphoreCount = waitSemaphores.Size();
	info.pWaitSemaphores = waitSemaphores.Data();
	info.pWaitDstStageMask = waitStageMasks.Data();
	
	// collect cmds and signal semaphores.
	TArray<VkSemaphore> semaphores; semaphores.Reserve(cmds.Size());
	TArray<VkPipelineStageFlags> pipelineStageMasks; pipelineStageMasks.Reserve(cmds.Size());
	TArray<VulkanCommandBuffer*> recordCmds; recordCmds.Reserve(cmds.Size());
	const uint32 cmdCount = cmds.Size();
	TFixedArray<VkCommandBuffer> vkCmds(cmds.Size());
	for(uint32 i=0; i<cmdCount; ++i) {
		auto cmd = cmds[i];
		cmd->CheckEnd();
		vkCmds[i] = cmd->m_Handle;
		if (cmd->m_Semaphore) {
			semaphores.PushBack(cmd->m_Semaphore);
			pipelineStageMasks.PushBack(cmd->m_StageMask);
			recordCmds.PushBack(cmd);
		}
	}
	info.commandBufferCount = cmdCount;
	info.pCommandBuffers = vkCmds.Data();
	info.signalSemaphoreCount = semaphores.Size();
	info.pSignalSemaphores = semaphores.Data();
	vkQueueSubmit(m_Device->GetGraphicsQueue().Handle, 1, &info, fence);

	// record cmds for the next submission or present
	auto& submission = m_Submissions.EmplaceBack();
	for(auto* cmd: cmds) {
		if(cmd->m_Semaphore) {
			submission.PushBack(cmd);
		}
	}
}

void VulkanCommandContext::BeginFrame() {
	// clear submission cache
	m_Submissions.Reset();
	// submit upload cmd if need.
	if(m_UploadCmd->m_HasBegun) {
		m_UploadCmd->CheckEnd();
		SubmitCommandBuffer(m_UploadCmd.Get(), VK_NULL_HANDLE, VK_NULL_HANDLE);
	}
}

const VulkanCommandSubmission& VulkanCommandContext::GetLastSubmission() {
	static VulkanCommandSubmission s_EmptySubmission{};
	return m_Submissions.IsEmpty() ? s_EmptySubmission : m_Submissions.Back();
}

const void VulkanCommandContext::GetLastSubmissionSemaphores(TArray<VkSemaphore>& outSmps) {
	outSmps.Reset();
	auto& lastSubmissions = GetLastSubmission();
	if(lastSubmissions.IsEmpty()) {
		return;
	}
	outSmps.Reserve(lastSubmissions.Size());
	for(auto* cmd: lastSubmissions) {
		if(cmd->m_Semaphore) {
			outSmps.PushBack(cmd->m_Semaphore);
		}
	}
}

VulkanCommandBuffer* VulkanCommandContext::GetUploadCmd() {
	m_UploadCmd->CheckBegin();
	return m_UploadCmd.Get();
}

VulkanDevice* VulkanCommandContext::GetDevice() {
	return m_Device;
}
