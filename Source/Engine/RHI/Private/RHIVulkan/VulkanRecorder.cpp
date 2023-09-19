#include "VulkanRecorder.h"
namespace Engine {

	void RHIRecorder::AddAllocateDescriptorSetInfo(RDescriptorSetVk** descPtr, VkDescriptorSetLayout layoutHandle)
	{
		m_AllocateDescs.PushBack(descPtr);
		m_AllocateDescLayouts.PushBack(layoutHandle);
	}

	void RHIRecorder::AllocateDescriptorSets(VkDevice device, VkDescriptorPool pool)
	{
		if (!m_AllocateDescs.Empty()) {
			VkDescriptorSetAllocateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr };
			info.descriptorPool = pool;
			info.descriptorSetCount = static_cast<uint32>(m_AllocateDescs.Size());
			info.pSetLayouts = m_AllocateDescLayouts.Data();
			TVector<VkDescriptorSet> handles(m_AllocateDescs.Size());
			if (VK_SUCCESS != vkAllocateDescriptorSets(device, &info, handles.Data())) {
				for (auto& ptr : m_AllocateDescs) {
					delete *ptr; *ptr = nullptr;
				}
			}
			else {
				for (uint32 i = 0; i < m_AllocateDescs.Size(); ++i) {
					(*m_AllocateDescs[i])->handle = handles[i];
				}
			}
			m_AllocateDescs.Clear();
			m_AllocateDescLayouts.Clear();
		}
	}

	void RHIRecorder::UpdateDescriptorSets(VkDevice device)
	{

	}

	void RHIRecorder::AddFreeDescriptorSetInfo(VkDescriptorSet descriptorSet)
	{
		m_FreeDescs.PushBack(descriptorSet);
	}

	void RHIRecorder::FreeDescriptorSets(VkDevice device, VkDescriptorPool pool)
	{
		if (!m_FreeDescs.Empty()) {
			vkFreeDescriptorSets(device, pool, m_FreeDescs.Size(), m_FreeDescs.Data());
			m_FreeDescs.Clear();
		}
	}
	void RHIRecorder::AddAllocateCommandBufferInfo(VkCommandBufferLevel level, RCommandBufferVk** cmd)
	{
		m_AllocateCmdLevels.PushBack(level);
		m_AllocateCmds.PushBack(cmd);
	}

	void RHIRecorder::AllocateCommandBuffer(VkDevice device, VkCommandPool pool)
	{
		if(!m_AllocateCmds.Empty()) {
			TVector<VkCommandBuffer> primaryCmds;
			TVector<VkCommandBuffer> secondaryCmds;
			for(auto level: m_AllocateCmdLevels) {
				if(level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
					primaryCmds.PushBack(VK_NULL_HANDLE);
				}
				else if(level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
					secondaryCmds.PushBack(VK_NULL_HANDLE);
				}
			}
			if(!primaryCmds.Empty()) {
				VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
					pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32)primaryCmds.Size()};
				vkAllocateCommandBuffers(device, &allocateInfo, primaryCmds.Data());
			}
			if(!secondaryCmds.Empty()) {
				VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
					pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY, (uint32)secondaryCmds.Size() };
				vkAllocateCommandBuffers(device, &allocateInfo, secondaryCmds.Data());
			}
			uint32 primaryIdx{0}, secondaryIdx{0};
			for(uint32 i=0; i<m_AllocateCmdLevels.Size(); ++i) {
				if (m_AllocateCmdLevels[i] == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
					(*m_AllocateCmds[i])->handle = primaryCmds[primaryIdx++];
				}
				else if(m_AllocateCmdLevels[i] == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
					(*m_AllocateCmds[i])->handle = secondaryCmds[secondaryIdx++];
				}
			}
		}
	}

	void RHIRecorder::AddFreeCommandBuffer(VkCommandBuffer cmd)
	{
		m_FreeCmds.PushBack(cmd);
	}

	void RHIRecorder::FreeCommandBuffers(VkDevice device, VkCommandPool pool) {
		if (!m_FreeCmds.Empty()) {
			vkFreeCommandBuffers(device, pool, m_FreeCmds.Size(), m_FreeCmds.Data());
			m_FreeCmds.Clear();
		}
	}
}