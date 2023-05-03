#pragma once
#include "VulkanUtil.h"

namespace Engine {
	class RHIRecorder {
	private:
		// VkAllocateDescriptorSets
		TVector<RDescriptorSetVk**> m_AllocateDescs;
		TVector<VkDescriptorSetLayout> m_AllocateDescLayouts;

		// VkUpdateDescriptorSets
		struct SUpdateDescsInfo {
			VkDescriptorSet handle;
			uint32 binding;
			uint32 arrayElement;
			uint32 elementCount;
			VkDescriptorType type;
			RDescriptorInfo info;
		};
		TVector<SUpdateDescsInfo> m_UpdateDescs;

		// VkFreedDescriptorSets
		TVector<VkDescriptorSet> m_FreeDescs;

		// VkAllocateCommandBuffers
		TVector<RCommandBufferVk**> m_AllocateCmds;
		TVector<VkCommandBufferLevel> m_AllocateCmdLevels;

		// VkFreeCommandBuffers
		TVector<VkCommandBuffer> m_FreeCmds;
	public:
		void AddAllocateDescriptorSetInfo(RDescriptorSetVk** descPtr, VkDescriptorSetLayout layoutHandle);
		void AllocateDescriptorSets(VkDevice device, VkDescriptorPool pool);
		void UpdateDescriptorSets(VkDevice device);

		void AddFreeDescriptorSetInfo(VkDescriptorSet descriptorSet);
		void FreeDescriptorSets(VkDevice device, VkDescriptorPool pool);

		void AddAllocateCommandBufferInfo(VkCommandBufferLevel level, RCommandBufferVk** cmd);
		void AllocateCommandBuffer(VkDevice device, VkCommandPool pool);
		void AddFreeCommandBuffer(VkCommandBuffer cmd);
		void FreeCommandBuffers(VkDevice device, VkCommandPool pool);
	};
}