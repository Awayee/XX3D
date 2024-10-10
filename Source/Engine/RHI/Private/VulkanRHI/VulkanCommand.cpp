#include "VulkanCommand.h"
#include "VulkanResources.h"
#include "VulkanConverter.h"
#include "VulkanPipeline.h"
#include "Core/Public/TArray.h"
#include "VulkanDevice.h"
#include "System/Public/FrameCounter.h"

namespace {
	void GetPipelineBarrierStage(VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags& srcAccessMask, VkAccessFlags& dstAccessMask, VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage) {
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			srcAccessMask = 0;
			dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
			srcAccessMask = 0;
			dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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
		for (uint32 i = 1; i < levelCount; i++){
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



VulkanStagingBuffer::VulkanStagingBuffer(uint32 bufferSize, VulkanDevice* device) : VulkanBuffer(RHIBufferDesc{ EBufferFlags::CopySrc, bufferSize }, device) {
	m_CreateFrame = FrameCounter::GetFrame();
}

VulkanUploader::VulkanUploader(VulkanDevice* device) : m_Device(device) {
}

VulkanUploader::~VulkanUploader() {
}

VulkanStagingBuffer* VulkanUploader::AcquireBuffer(uint32 bufferSize) {
	auto& bufferPtr = m_StagingBuffers.EmplaceBack(new VulkanStagingBuffer(bufferSize, m_Device));
	bufferPtr->SetName("UploadBuffer");
	return bufferPtr.Get();
}

void VulkanUploader::BeginFrame() {
	const uint32 frame = FrameCounter::GetFrame();
	for (uint32 i = 0; i < m_StagingBuffers.Size(); ) {
		const uint32 createFrame = m_StagingBuffers[i]->GetCreateFrame();
		if (createFrame + RHI_FRAME_IN_FLIGHT_MAX < frame) {
			m_StagingBuffers.SwapRemoveAt(i);
		}
		else {
			++i;
		}
	}
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandContext* context, VkCommandBuffer handle, VkSemaphore smp, EQueueType queue) :
m_Owner(context),
m_Handle(handle),
m_Semaphore(smp),
m_QueueType(queue),
// TODO stage mask would be reset
m_StageMask(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT){
	m_PipelineDescriptorSetCache.Reset(new VulkanPipelineDescriptorSetCache(context->GetDevice()));
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
	ASSERT(!m_HasBegun, "Command buffer must be Destroyed after ending.");
	m_Owner->FreeCommandBuffer(this);
}

void VulkanCommandBuffer::Reset() {
	vkResetCommandBuffer(m_Handle, 0);
	m_PipelineDescriptorSetCache->Reset();
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
		RHITextureSubRes subRes{target.ArrayIndex, 1, target.MipIndex, 1, ETextureDimension::Tex2D, ETextureViewFlags::Color};
		colorAttachments[i].imageView = ((VulkanRHITexture*)target.Target)->GetView(subRes);
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
		RHITextureSubRes subRes{depthStencilTarget.ArrayIndex, 1, depthStencilTarget.MipIndex, 1, ETextureDimension::Tex2D, ETextureViewFlags::Depth|ETextureViewFlags::Stencil};
		dsAttachment.imageView = ((VulkanRHITexture*)depthStencilTarget.Target)->GetView(subRes);
		dsAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		dsAttachment.loadOp = ToVkAttachmentLoadOp(depthStencilTarget.DepthLoadOp);
		dsAttachment.storeOp = ToVkAttachmentStoreOp(depthStencilTarget.DepthStoreOp);
		dsAttachment.clearValue.depthStencil.depth = depthStencilTarget.DepthClear;
		dsAttachment.clearValue.depthStencil.stencil = depthStencilTarget.StencilClear;
		renderingInfo.pDepthAttachment = &dsAttachment;
		renderingInfo.pStencilAttachment = &dsAttachment;
	}

	vkCmdBeginRendering(m_Handle, &renderingInfo);
	m_Viewport = { (float)area.x, (float)area.y + (float)area.h, (float)area.w, -(float)area.h, 0.0f, 1.0f };
	m_Scissor = { {area.x, area.y}, {area.w, area.h} };
}

void VulkanCommandBuffer::EndRendering() {
	vkCmdEndRendering(m_Handle);
}

void VulkanCommandBuffer::BindGraphicsPipeline(RHIGraphicsPipelineState* pipeline) {
	VulkanRHIGraphicsPipelineState* vkPipeline = static_cast<VulkanRHIGraphicsPipelineState*>(pipeline);
	VkPipeline handle = vkPipeline->GetPipelineHandle();
	if(m_PipelineDescriptorSetCache->GetVkPipeline() != handle) {
		m_PipelineDescriptorSetCache->BindGraphicsPipeline(vkPipeline);
		vkCmdBindPipeline(m_Handle, VK_PIPELINE_BIND_POINT_GRAPHICS, handle);
	}
}

void VulkanCommandBuffer::BindComputePipeline(RHIComputePipelineState* pipeline) {
	VulkanRHIComputePipelineState* vkPipeline = static_cast<VulkanRHIComputePipelineState*>(pipeline);
	VkPipeline handle = vkPipeline->GetPipelineHandle();
	if(m_PipelineDescriptorSetCache->GetVkPipeline() != handle) {
		m_PipelineDescriptorSetCache->BindComputePipeline(vkPipeline);
		vkCmdBindPipeline(m_Handle, VK_PIPELINE_BIND_POINT_COMPUTE, handle);
	}
}

void VulkanCommandBuffer::SetShaderParam(uint32 setIndex, uint32 bindIndex, const RHIShaderParam& parameter) {
	m_PipelineDescriptorSetCache->SetParam(setIndex, bindIndex, parameter);
}

void VulkanCommandBuffer::BindVertexBuffer(RHIBuffer* buffer, uint32 slot, uint64 offset) {
	VkBuffer vkBuffer = static_cast<VulkanBufferImpl*>(buffer)->GetBuffer();
	vkCmdBindVertexBuffers(m_Handle, slot, 1, &vkBuffer, &offset);
}

void VulkanCommandBuffer::BindVertexBuffer(const RHIDynamicBuffer& buffer, uint32 slot, uint32 offset) {
	VkBuffer vkBuffer = m_Owner->GetDevice()->GetDynamicBufferAllocator()->GetBufferHandle(buffer.BufferIndex);
	VkDeviceSize bufferOffset = (VkDeviceSize)(buffer.Offset + offset);
	vkCmdBindVertexBuffers(m_Handle, slot, 1, &vkBuffer, &bufferOffset);
}

void VulkanCommandBuffer::BindIndexBuffer(RHIBuffer* buffer, uint64 offset) {
	vkCmdBindIndexBuffer(m_Handle, static_cast<VulkanBufferImpl*>(buffer)->GetBuffer(), offset, ToIndexType(buffer->GetDesc().IndexFormat));
}

void VulkanCommandBuffer::SetViewport(FRect rect, float minDepth, float maxDepth) {
	m_Viewport = { rect.x, rect.y + rect.h, rect.w, -rect.h, minDepth, maxDepth };
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

void VulkanCommandBuffer::CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint32 srcOffset, uint32 dstOffset, uint32 byteSize) {
	VkBufferCopy copy{ srcOffset, dstOffset, byteSize };
	vkCmdCopyBuffer(m_Handle, static_cast<VulkanBufferImpl*>(srcBuffer)->GetBuffer(), static_cast<VulkanBufferImpl*>(dstBuffer)->GetBuffer(), 1, &copy);
}

void VulkanCommandBuffer::CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, RHITextureSubRes dstSubRes, IOffset3D dstOffset) {
	VulkanRHITexture* vkTex = static_cast<VulkanRHITexture*>(texture);
	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	ToImageSubResourceLayers(dstSubRes, region.imageSubresource);
	region.imageOffset = { dstOffset.x, dstOffset.y, dstOffset.z };
	const auto& desc = vkTex->GetDesc();
	region.imageExtent = { desc.GetMipLevelWidth(dstSubRes.MipIndex), desc.GetMipLevelHeight(dstSubRes.MipIndex), desc.Depth };
	vkCmdCopyBufferToImage(m_Handle, ((VulkanBufferImpl*)buffer)->GetBuffer(), vkTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void VulkanCommandBuffer::CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RHITextureCopyRegion& region){
	VkImageCopy copy{};
	ToImageSubResourceLayers(region.SrcSubRes, copy.srcSubresource);
	ToImageSubResourceLayers(region.DstSubRes, copy.dstSubresource);
	copy.srcOffset = { (int)region.SrcOffset.x, (int)region.SrcOffset.y, (int)region.SrcOffset.z };
	copy.dstOffset = { (int)region.DstOffset.x, (int)region.DstOffset.y, (int)region.DstOffset.z };
	copy.extent = { region.Extent.w, region.Extent.h, region.Extent.d };
	vkCmdCopyImage(m_Handle, ((VulkanRHITexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ((VulkanRHITexture*)dstTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
}

void VulkanCommandBuffer::TransitionTextureState(RHITexture* texture, EResourceState stateBefore, EResourceState stateAfter, RHITextureSubRes subRes) {
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
	ToImageSubResourceRange(subRes, barrier.subresourceRange);
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;
	GetPipelineBarrierStage(barrier.oldLayout, barrier.newLayout, barrier.srcAccessMask, barrier.dstAccessMask, srcStage, dstStage);
	vkCmdPipelineBarrier(m_Handle, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanCommandBuffer::GenerateMipmap(RHITexture* texture, uint8 mipSize, uint16 arrayIndex, uint16 arraySize, ETextureViewFlags viewFlags) {
	VulkanRHITexture* vkTex = static_cast<VulkanRHITexture*>(texture);
	const auto& desc = vkTex->GetDesc();
	const VkImageAspectFlags aspect = ToImageAspectFlags(viewFlags);
	GenerateMipMap(m_Handle, vkTex->GetImage(), mipSize, desc.Width, desc.Height, aspect, arrayIndex, arraySize);
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

void VulkanCommandBuffer::PrepareDraw(){
	m_PipelineDescriptorSetCache->Bind(m_Handle);
	if(VK_PIPELINE_BIND_POINT_GRAPHICS == m_PipelineDescriptorSetCache->GetVkPipelineBindPoint()) {
		if (m_ViewportDirty) {
			vkCmdSetViewport(m_Handle, 0, 1, &m_Viewport);
			m_ViewportDirty = false;
		}
		if (m_ScissorDirty) {
			vkCmdSetScissor(m_Handle, 0, 1, &m_Scissor);
			m_ScissorDirty = false;
		}
	}
}

VulkanCommandContext::VulkanCommandContext(VulkanDevice* device) :m_Device(device) {
	VkDevice vkDevice = m_Device->GetDevice();
	for(uint8 i=0; i<EnumCast(EQueueType::Count); ++i) {
		VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr };
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = m_Device->GetQueue((EQueueType)i).FamilyIndex;
		VK_ASSERT(vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &m_CommandPools[i]), "VkCreateCommandPool");
	}
	for(uint8 i=0; i<EnumCast(EQueueType::Count); ++i) {
		m_UploadCmds[i] = AllocateCommandBuffer((EQueueType)i);
	}
}

VulkanCommandContext::~VulkanCommandContext() {
	// cmds must be freed before command pool destroyed.
	for(auto& cmd: m_UploadCmds) {
		cmd.Reset();
	}
	for(VkCommandPool pool: m_CommandPools) {
		vkDestroyCommandPool(m_Device->GetDevice(), pool, nullptr);
	}
}

RHICommandBufferPtr VulkanCommandContext::AllocateCommandBuffer(EQueueType queue) {
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr };
	allocateInfo.commandPool = GetCommandPool(queue);
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;
	VkCommandBuffer handle;
	VK_CHECK(vkAllocateCommandBuffers(m_Device->GetDevice(), &allocateInfo, &handle));
	VkSemaphore smp;
	VkSemaphoreCreateInfo semaphoreInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
	vkCreateSemaphore(m_Device->GetDevice(), &semaphoreInfo, nullptr, &smp);
	return RHICommandBufferPtr(new VulkanCommandBuffer(this, handle, smp, queue));
}

void VulkanCommandContext::FreeCommandBuffer(VulkanCommandBuffer* cmd) {
	VkCommandPool pool = GetCommandPool(cmd->GetQueueType());
	vkFreeCommandBuffers(m_Device->GetDevice(), pool, 1, &cmd->m_Handle);
	vkDestroySemaphore(m_Device->GetDevice(), cmd->m_Semaphore, nullptr);
}

void VulkanCommandContext::SubmitCommandBuffers(TArrayView<VulkanCommandBuffer*> cmds, EQueueType queue, VkSemaphore waitSemaphore, VkFence fence) {
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
	vkQueueSubmit(m_Device->GetQueue(queue).Handle, 1, &info, fence);

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
	for(uint8 i=0; i<EnumCast(EQueueType::Count); ++i) {
		VulkanCommandBuffer* cmd = m_UploadCmds[i];
		if(cmd->m_HasBegun) {
			cmd->CheckEnd();
			SubmitCommandBuffers({ cmd }, (EQueueType)i, VK_NULL_HANDLE, VK_NULL_HANDLE);
		}
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

VulkanCommandBuffer* VulkanCommandContext::GetUploadCmd(EQueueType queue) {
	VulkanCommandBuffer* cmd = m_UploadCmds[EnumCast(queue)].Get();
	cmd->CheckBegin();
	return cmd;
}

VulkanDevice* VulkanCommandContext::GetDevice() {
	return m_Device;
}

VkCommandPool VulkanCommandContext::GetCommandPool(EQueueType queue) {
	return m_CommandPools[EnumCast(queue)];
}
