#include "RHIVKResources.h"
#include "VulkanUtil.h"
#include "RHIVulkan.h"
namespace Engine {
    void RRenderPassVk::SetAttachment(uint32 idx, RImageView* imageView){
        if(!(idx < m_Attachments.Size())) {
	        for(int i= m_Attachments.Size()-1; i<idx; ++i) {
                m_Attachments.PushBack(VK_NULL_HANDLE);
	        }
        }
        m_Attachments[idx] = ((RImageViewVk*)imageView)->handle;
    }
    void RRenderPassVk::SetClearValue(uint32 idx, const RSClear& clear){
        if(!(idx < m_Clears.Size())) {
	        for(int i=m_Clears.Size()-1;i<idx; ++i) {
                m_Clears.PushBack({});
	        }
        }
        m_Clears[idx] = ResolveClearValue(clear);
    }


	void RDescriptorSetVk::InnerUpdate(uint32 binding, uint32 arrayElement, uint32 count, VkDescriptorType type, const VkDescriptorImageInfo* imageInfo, const VkDescriptorBufferInfo* bufferInfo, const VkBufferView* texelBufferView){
		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, handle };
		write.dstBinding = binding;
		write.dstArrayElement = arrayElement;
		write.descriptorCount = count;
		write.descriptorType = type;
		write.pImageInfo = imageInfo;
		write.pBufferInfo = bufferInfo;
		write.pTexelBufferView = texelBufferView;
		vkUpdateDescriptorSets(RHIVulkan::InstanceVulkan()->GetDevice(), 1, &write, 0, nullptr);
	}

	void RDescriptorSetVk::Update(uint32 binding, RDescriptorType type, const RDescriptorInfo& info,  uint32 arrayElement, uint32 count){
		VkDescriptorType vkType = (VkDescriptorType)type;
		switch (vkType) {
			// buffer
		case (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER):
		case (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER):
		case (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC):
		case (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC): {
			VkDescriptorBufferInfo bufferInfo{ ((RBufferVk*)info.buffer)->handle, info.offset, info.range };
			InnerUpdate(binding, arrayElement, count, vkType, nullptr, &bufferInfo, nullptr);
			break;
		}

		// image
		case (VK_DESCRIPTOR_TYPE_SAMPLER):
		case (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER):
		case (VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE):
		case (VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT):
		case (VK_DESCRIPTOR_TYPE_STORAGE_IMAGE): {
			VkDescriptorImageInfo imageInfo{};
			if (nullptr != info.sampler) {
				imageInfo.sampler = ((RSamplerVk*)info.sampler)->handle;
			}
			if (nullptr != info.imageView) {
				RImageViewVk* imageViewVk = (RImageViewVk*)info.imageView;
				imageInfo.imageView = imageViewVk->handle;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			InnerUpdate(binding, arrayElement, count, vkType, &imageInfo, nullptr, nullptr);
			break;
		}

		// buffer view TODO
		case (VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER):
		case (VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER): { }
		default: break;
		}
    }


    void RDescriptorSetVk::SetUniformBuffer(uint32 binding, RBuffer* buffer){
		VkDescriptorBufferInfo bufferInfo{ ((RBufferVk*)buffer)->handle, 0, buffer->GetSize() };
		InnerUpdate(binding, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &bufferInfo, nullptr);
    }

    void RDescriptorSetVk::SetImageSampler(uint32 binding, RSampler* sampler, RImageView* image){
		VkDescriptorImageInfo imageInfo{ ((RSamplerVk*)sampler)->handle, ((RImageViewVk*)image)->handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		InnerUpdate(binding, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo, nullptr, nullptr);
    }

    void RDescriptorSetVk::SetImage(uint32 binding, RImageView* image) {
		VkDescriptorImageInfo imageInfo{ nullptr, ((RImageViewVk*)image)->handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		InnerUpdate(binding, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, nullptr, nullptr);
    }

    void RDescriptorSetVk::SetSampler(uint32 binding, RSampler* sampler) {
		VkDescriptorImageInfo imageInfo{ ((RSamplerVk*)sampler)->handle, nullptr, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		InnerUpdate(binding, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, nullptr, nullptr);
    }

    void RDescriptorSetVk::SetInputAttachment(uint32 binding, RImageView* image){
		VkDescriptorImageInfo imageInfo{ VK_NULL_HANDLE, ((RImageViewVk*)image)->handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		InnerUpdate(binding, 0, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo, nullptr, nullptr);
    }


	void RCommandBufferVk::Begin(RCommandBufferUsageFlags flags){
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
		beginInfo.flags = flags;
		beginInfo.pInheritanceInfo = nullptr;
		vkBeginCommandBuffer(handle, &beginInfo);
	}

	void RCommandBufferVk::End(){
		vkEndCommandBuffer(handle);
	}

	void RCommandBufferVk::BeginRenderPass(RRenderPass* pass, RFramebuffer* framebuffer, const URect& area){
		RRenderPassVk* passVk = reinterpret_cast<RRenderPassVk*>(pass);
		VkRect2D vkRenderArea{ {(int32)area.x, (int32)area.y}, {area.w, area.h} };
		VkRenderPassBeginInfo passInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		passInfo.pNext = nullptr;
		passInfo.renderPass = passVk->handle;
		passInfo.framebuffer = reinterpret_cast<RFramebufferVk*>(framebuffer)->handle;
		passInfo.renderArea = vkRenderArea;
		passInfo.clearValueCount = static_cast<uint32>(passVk->GetClears().Size());
		passInfo.pClearValues = passVk->GetClears().Data();
		vkCmdBeginRenderPass(handle, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void RCommandBufferVk::NextSubpass(){
		vkCmdNextSubpass(handle, VK_SUBPASS_CONTENTS_INLINE);
	}

	void RCommandBufferVk::EndRenderPass(){
		vkCmdEndRenderPass(handle);
	}

	void RCommandBufferVk::CopyBufferToImage(RBuffer* buffer, RImage* image, RImageAspectFlags aspect, uint32 mipLevel, uint32 baseLayer, uint32 layerCount){
		RImageVk* imageVk = (RImageVk*)image;
		VkBufferImageCopy region;
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = mipLevel;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { imageVk->GetExtent().w, imageVk->GetExtent().h, imageVk->GetExtent().d };
		vkCmdCopyBufferToImage(handle, ((RBufferVk*)buffer)->handle, imageVk->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	void RCommandBufferVk::BlitImage(RCommandBuffer* cmd, RImage* srcImage, RImage* dstImage, const RSImageBlit& region)
	{
		VkImageBlit blit{};
		blit.srcSubresource.aspectMask = region.srcAspect;
		blit.srcSubresource.baseArrayLayer = region.srcBaseLayer;
		blit.srcSubresource.layerCount = region.srcLayerCount;
		memcpy(blit.srcOffsets, region.srcOffsets, sizeof(VkOffset3D) * 2);
		blit.dstSubresource.aspectMask = region.dstAspect;
		blit.dstSubresource.baseArrayLayer = region.dstBaseLayer;
		blit.dstSubresource.layerCount = region.dstLayerCount;
		memcpy(blit.dstOffsets, region.dstOffsets, sizeof(VkOffset3D) * 2);
		vkCmdBlitImage(((RCommandBufferVk*)cmd)->handle, ((RImageVk*)srcImage)->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			((RImageVk*)dstImage)->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
	}

	void RCommandBufferVk::BindPipeline(RPipeline* pipeline){
		vkCmdBindPipeline(handle, (VkPipelineBindPoint)pipeline->GetType(), ((RPipelineVk*)pipeline)->handle);
	}

	void RCommandBufferVk::BindDescriptorSet(RPipelineLayout* layout, RDescriptorSet* descriptorSet, uint32 setIdx, RPipelineType pipelineType){
		vkCmdBindDescriptorSets(handle, (VkPipelineBindPoint)pipelineType, ((RPipelineLayoutVk*)layout)->handle, setIdx, 1, &((RDescriptorSetVk*)descriptorSet)->handle, 0, nullptr);
	}

	void RCommandBufferVk::BindVertexBuffer(RBuffer* buffer, uint32 first, uint64 offset){
		vkCmdBindVertexBuffers(handle, first, 1, &((RBufferVk*)buffer)->handle, &offset);
	}

	void RCommandBufferVk::BindIndexBuffer(RBuffer* buffer, uint64 offset){
		vkCmdBindIndexBuffer(handle, ((RBufferVk*)buffer)->handle, offset, VK_INDEX_TYPE_UINT32);
	}

	void RCommandBufferVk::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance){
		vkCmdDraw(handle, vertexCount, instanceCount, firstIndex, firstInstance);
	}

	void RCommandBufferVk::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance){
		vkCmdDrawIndexed(handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void RCommandBufferVk::DrawPrimitive(RBuffer* buffer, uint32 vertexCount, uint32 instanceCount){
		constexpr VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(handle, 0, 1, &((RBufferVk*)buffer)->handle, &offset);
		vkCmdDraw(handle, vertexCount, instanceCount, 0, 0);
	}

	void RCommandBufferVk::DrawPrimitiveIndexed(RBuffer* vertexBuffer, RBuffer* indexBuffer, uint32 indexCount, uint32 instanceCount){
		constexpr VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(handle, 0, 1, &((RBufferVk*)vertexBuffer)->handle, &offset);
		vkCmdBindIndexBuffer(handle, ((RBufferVk*)indexBuffer)->handle, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(handle, indexCount, instanceCount, 0, 0, 0);
	}

	void RCommandBufferVk::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ){
		vkCmdDispatch(handle, groupCountX, groupCountY, groupCountZ);
	}

	void RCommandBufferVk::ClearAttachment(RImageAspectFlags aspect, const float* color, const URect& rect){
		VkClearAttachment clearAttachment;
		clearAttachment.aspectMask = aspect;
		clearAttachment.clearValue.color.float32[0] = color[0];
		clearAttachment.clearValue.color.float32[1] = color[1];
		clearAttachment.clearValue.color.float32[2] = color[2];
		clearAttachment.clearValue.color.float32[3] = color[3];
		VkClearRect clearRect;
		clearRect.rect.extent.width = rect.w;
		clearRect.rect.extent.height = rect.h;
		clearRect.rect.offset.x = rect.x;
		clearRect.rect.offset.y = rect.y;
		clearRect.baseArrayLayer = 0;
		clearRect.layerCount = 1;
		vkCmdClearAttachments(handle, 1, &clearAttachment, 1, &clearRect);
	}

	void RCommandBufferVk::CopyBuffer(RBuffer* srcBuffer, RBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size){
		VkBufferCopy copy{ srcOffset, dstOffset, size };
		vkCmdCopyBuffer(handle, ((RBufferVk*)srcBuffer)->handle, ((RBufferVk*)dstBuffer)->handle, 1, &copy);
	}

	void RCommandBufferVk::TransitionImageLayout(RImage* image, RImageLayout oldLayout, RImageLayout newLayout, uint32 baseMipLevel, uint32 levelCount, uint32 baseLayer, uint32 layerCount, RImageAspectFlags aspect){
		RImageVk* imageVk = (RImageVk*)image;
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = (VkImageLayout)oldLayout;
		barrier.newLayout = (VkImageLayout)newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = imageVk->handle;
		barrier.subresourceRange.aspectMask = aspect;
		barrier.subresourceRange.baseMipLevel = baseMipLevel;
		barrier.subresourceRange.levelCount = levelCount;
		barrier.subresourceRange.baseArrayLayer = baseLayer;
		barrier.subresourceRange.layerCount = layerCount;
		VkPipelineStageFlags srcStage;
		VkPipelineStageFlags dstStage;
		GetPipelineBarrierStage(barrier.oldLayout, barrier.newLayout, barrier.srcAccessMask, barrier.dstAccessMask, srcStage, dstStage);
		vkCmdPipelineBarrier(handle, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		imageVk->SetLayout(newLayout);
    }

	void RCommandBufferVk::GenerateMipmap(RImage* image, uint32 levelCount, RImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount) {
		RImageVk* imageVk = (RImageVk*)image;
		GenerateMipMap(handle, imageVk->handle, levelCount, imageVk->GetExtent().w, imageVk->GetExtent().h, aspect, baseLayer, layerCount);
	}

	void RCommandBufferVk::BeginDebugLabel(const char* msg, const float* color) {
		if (nullptr != vkCmdBeginDebugUtilsLabelEXT) {
			VkDebugUtilsLabelEXT labelInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr };
			labelInfo.pLabelName = msg;
			if (nullptr != color) {
				for (int i = 0; i < 4; ++i) {
					labelInfo.color[i] = color[i];
				}
			}
			vkCmdBeginDebugUtilsLabelEXT(handle, &labelInfo);
		}
	}

	void RCommandBufferVk::EndDebugLabel(){
		if(nullptr != vkCmdEndDebugUtilsLabelEXT) {
			vkCmdEndDebugUtilsLabelEXT(handle);
		}
	}
}