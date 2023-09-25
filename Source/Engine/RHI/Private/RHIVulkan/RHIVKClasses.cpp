#include "RHIVKClasses.h"
#include "VulkanUtil.h"
#include "RHIVulkan.h"
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
