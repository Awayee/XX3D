#include "RHIVKClasses.h"
#include "VulkanUtil.h"
#include "RHIVulkan.h"
#include "VUlkanFuncs.h"
#include "VulkanBuilder.h"

namespace Engine {

	void RRenderPassVk::SetAttachment(uint32 idx, RHITexture* texture){
        if(!(idx < m_Attachments.Size())) {
	        for(int i= m_Attachments.Size()-1; i<idx; ++i) {
                m_Attachments.PushBack(VK_NULL_HANDLE);
	        }
        }
        m_Attachments[idx] = ((RHIVkTexture*)texture)->GetView();
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
		vkUpdateDescriptorSets(RHIVulkan::GetDevice(), 1, &write, 0, nullptr);
	}

	/*
	void RDescriptorSetVk::Update(uint32 binding, RDescriptorType type, const RDescriptorInfo& info,  uint32 arrayElement, uint32 count){
		VkDescriptorType vkType = (VkDescriptorType)type;
		switch (vkType) {
			// buffer
		case (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER):
		case (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER):
		case (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC):
		case (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC): {
			VkDescriptorBufferInfo bufferInfo{ ((*)info.buffer)->handle, info.offset, info.range };
			InnerUpdate(binding, arrayElement, count, vkType, nullptr, &bufferInfo, nullptr);
			break;
		}

		// texture
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
    */


    void RDescriptorSetVk::SetUniformBuffer(uint32 binding, RHIBuffer* buffer){
		VkDescriptorBufferInfo bufferInfo{ dynamic_cast<RHIVkBuffer*>(buffer)->GetBuffer(), 0, buffer->GetDesc().ByteSize};
		InnerUpdate(binding, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &bufferInfo, nullptr);
    }

    void RDescriptorSetVk::SetImageSampler(uint32 binding, RHISampler* sampler, RHITexture* texture){
		VkDescriptorImageInfo imageInfo{ ((RHIVkSampler*)sampler)->GetSampler(), dynamic_cast<RHIVkTexture*>(texture)->GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		InnerUpdate(binding, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo, nullptr, nullptr);
    }

    void RDescriptorSetVk::SetTexture(uint32 binding, RHITexture* texture) {
		VkDescriptorImageInfo imageInfo{ nullptr, ((RHIVkTexture*)texture)->GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		InnerUpdate(binding, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, &imageInfo, nullptr, nullptr);
    }

    void RDescriptorSetVk::SetSampler(uint32 binding, RHISampler* sampler) {
		VkDescriptorImageInfo imageInfo{ ((RHIVkSampler*)sampler)->GetSampler(), nullptr, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		InnerUpdate(binding, 0, 1, VK_DESCRIPTOR_TYPE_SAMPLER, &imageInfo, nullptr, nullptr);
    }

    void RDescriptorSetVk::SetInputAttachment(uint32 binding, RHITexture* texture){
		VkDescriptorImageInfo imageInfo{ VK_NULL_HANDLE, ((RHIVkTexture*)texture)->GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		InnerUpdate(binding, 0, 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo, nullptr, nullptr);
    }


	void RHIVulkanCommandBuffer::Begin(RCommandBufferUsageFlags flags){
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr };
		beginInfo.flags = flags;
		beginInfo.pInheritanceInfo = nullptr;
		vkBeginCommandBuffer(handle, &beginInfo);
	}

	void RHIVulkanCommandBuffer::End(){
		vkEndCommandBuffer(handle);
	}

	void RHIVulkanCommandBuffer::BeginRenderPass(RRenderPass* pass, RFramebuffer* framebuffer, const URect& area){
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

	void RHIVulkanCommandBuffer::NextSubpass(){
		vkCmdNextSubpass(handle, VK_SUBPASS_CONTENTS_INLINE);
	}

	void RHIVulkanCommandBuffer::EndRenderPass(){
		vkCmdEndRenderPass(handle);
	}

	void RHIVulkanCommandBuffer::CopyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset, uint64 size) {
		VkBufferCopy copy{ srcOffset, dstOffset, size };
		vkCmdCopyBuffer(handle, dynamic_cast<RHIVkBuffer*>(srcBuffer)->GetBuffer(), dynamic_cast<RHIVkBuffer*>(dstBuffer)->GetBuffer(), 1, &copy);
	}

	void RHIVulkanCommandBuffer::CopyBufferToTexture(RHIBuffer* buffer, RHITexture* texture, uint32 mipLevel, uint32 baseLayer, uint32 layerCount){
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
		region.imageExtent = { size.w, size.h, size.d};
		vkCmdCopyBufferToImage(handle, ((RHIVkBuffer*)buffer)->GetBuffer(), vkTex->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	void RHIVulkanCommandBuffer::CopyTextureToTexture(RHITexture* srcTex, RHITexture* dstTex, const RSTextureCopyRegion& region)
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
		vkCmdBlitImage(handle, ((RHIVkTexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ((RHIVkTexture*)srcTex)->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
	}

	void RHIVulkanCommandBuffer::TransitionTextureLayout(RHITexture* texture, RImageLayout oldLayout, RImageLayout newLayout, uint32 baseMipLevel, uint32 levelCount, uint32 baseLayer, uint32 layerCount, RImageAspectFlags aspect) {
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
		vkCmdPipelineBarrier(handle, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void RHIVulkanCommandBuffer::GenerateMipmap(RHITexture* texture, uint32 levelCount, RImageAspectFlags aspect, uint32 baseLayer, uint32 layerCount) {
		RHIVkTexture* vkTex = dynamic_cast<RHIVkTexture*>(texture);
		USize3D size = vkTex->GetDesc().Size;
		GenerateMipMap(handle, vkTex->GetImage(), levelCount, size.w, size.h, aspect, baseLayer, layerCount);
	}

	void RHIVulkanCommandBuffer::BindPipeline(RPipeline* pipeline){
		vkCmdBindPipeline(handle, (VkPipelineBindPoint)pipeline->GetType(), ((RPipelineVk*)pipeline)->handle);
	}

	void RHIVulkanCommandBuffer::BindDescriptorSet(RPipelineLayout* layout, RDescriptorSet* descriptorSet, uint32 setIdx, RPipelineType pipelineType){
		vkCmdBindDescriptorSets(handle, (VkPipelineBindPoint)pipelineType, ((RPipelineLayoutVk*)layout)->handle, setIdx, 1, &((RDescriptorSetVk*)descriptorSet)->handle, 0, nullptr);
	}

	void RHIVulkanCommandBuffer::BindVertexBuffer(RHIBuffer* buffer, uint32 first, uint64 offset){
		VkBuffer vkBuffer = dynamic_cast<RHIVkBuffer*>(buffer)->GetBuffer();
		vkCmdBindVertexBuffers(handle, first, 1, &vkBuffer, &offset);
	}

	void RHIVulkanCommandBuffer::BindIndexBuffer(RHIBuffer* buffer, uint64 offset){
		vkCmdBindIndexBuffer(handle, dynamic_cast<RHIVkBuffer*>(buffer)->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
	}

	void RHIVulkanCommandBuffer::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance){
		vkCmdDraw(handle, vertexCount, instanceCount, firstIndex, firstInstance);
	}

	void RHIVulkanCommandBuffer::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance){
		vkCmdDrawIndexed(handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void RHIVulkanCommandBuffer::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ){
		vkCmdDispatch(handle, groupCountX, groupCountY, groupCountZ);
	}

	void RHIVulkanCommandBuffer::ClearAttachment(RImageAspectFlags aspect, const float* color, const URect& rect){
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

	void RHIVulkanCommandBuffer::BeginDebugLabel(const char* msg, const float* color) {
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

	void RHIVulkanCommandBuffer::EndDebugLabel(){
		if(nullptr != vkCmdEndDebugUtilsLabelEXT) {
			vkCmdEndDebugUtilsLabelEXT(handle);
		}
	}
}
