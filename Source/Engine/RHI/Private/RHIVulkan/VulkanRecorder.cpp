#include "VulkanRecorder.h"
namespace RHI {

	void RHIRecorder::AddAllocateDescriptorSetInfo(RDescriptorSetVk** descPtr, VkDescriptorSetLayout layoutHandle)
	{
		m_AllocateDescs.emplace_back(descPtr);
		m_AllocateDescLayouts.emplace_back(layoutHandle);
	}

	void RHIRecorder::AllocateDescriptorSets(VkDevice device, VkDescriptorPool pool)
	{
		if (!m_AllocateDescs.empty()) {
			VkDescriptorSetAllocateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr };
			info.descriptorPool = pool;
			info.descriptorSetCount = static_cast<uint32>(m_AllocateDescs.size());
			info.pSetLayouts = m_AllocateDescLayouts.data();
			TArray<VkDescriptorSet> handles(m_AllocateDescs.size());
			if (VK_SUCCESS != vkAllocateDescriptorSets(device, &info, handles.Data())) {
				for (auto& ptr : m_AllocateDescs) {
					delete *ptr; *ptr = nullptr;
				}
			}
			else {
				for (uint32 i = 0; i < m_AllocateDescs.size(); ++i) {
					(*m_AllocateDescs[i])->handle = handles[i];
				}
			}
			m_AllocateDescs.clear();
			m_AllocateDescLayouts.clear();
		}
	}

	void RHIRecorder::UpdateDescriptorSets(VkDevice device)
	{

	}

	void RHIRecorder::AddFreeDescriptorSetInfo(VkDescriptorSet descriptorSet)
	{
		m_FreeDescs.push_back(descriptorSet);
	}

	void RHIRecorder::FreeDescriptorSets(VkDevice device, VkDescriptorPool pool)
	{
		if (!m_FreeDescs.empty()) {
			vkFreeDescriptorSets(device, pool, m_FreeDescs.size(), m_FreeDescs.data());
			m_FreeDescs.clear();
		}
	}
	void RHIRecorder::AddAllocateCommandBufferInfo(VkCommandBufferLevel level, RCommandBufferVk** cmd)
	{
		m_AllocateCmdLevels.emplace_back(level);
		m_AllocateCmds.emplace_back(cmd);
	}

	void RHIRecorder::AllocateCommandBuffer(VkDevice device, VkCommandPool pool)
	{
		if(!m_AllocateCmds.empty()) {
			TVector<VkCommandBuffer> primaryCmds;
			TVector<VkCommandBuffer> secondaryCmds;
			for(auto level: m_AllocateCmdLevels) {
				if(level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
					primaryCmds.push_back(VK_NULL_HANDLE);
				}
				else if(level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
					secondaryCmds.push_back(VK_NULL_HANDLE);
				}
			}
			if(!primaryCmds.empty()) {
				VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
					pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, (uint32)primaryCmds.size()};
				vkAllocateCommandBuffers(device, &allocateInfo, primaryCmds.data());
			}
			if(!secondaryCmds.empty()) {
				VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
					pool, VK_COMMAND_BUFFER_LEVEL_SECONDARY, (uint32)secondaryCmds.size() };
				vkAllocateCommandBuffers(device, &allocateInfo, secondaryCmds.data());
			}
			uint32 primaryIdx{0}, secondaryIdx{0};
			for(uint32 i=0; i<m_AllocateCmdLevels.size(); ++i) {
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
		m_FreeCmds.push_back(cmd);
	}

	void RHIRecorder::FreeCommandBuffers(VkDevice device, VkCommandPool pool) {
		if (!m_FreeCmds.empty()) {
			vkFreeCommandBuffers(device, pool, m_FreeCmds.size(), m_FreeCmds.data());
			m_FreeCmds.clear();
		}
	}
}