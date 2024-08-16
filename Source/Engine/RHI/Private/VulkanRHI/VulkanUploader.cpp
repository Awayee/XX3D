#include "VulkanUploader.h"
#include "VulkanCommand.h"
#include "VulkanDevice.h"
#include "System/Public/FrameCounter.h"

VulkanStagingBuffer::VulkanStagingBuffer(const RHIBufferDesc& desc, VulkanDevice* device) : RHIBuffer(desc), m_Device(device) {
	constexpr uint32 memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	m_Device->GetMemoryMgr()->AllocateBufferMemory(m_Allocation, m_Desc.ByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memoryProperty);
	m_CreateFrame = FrameCounter::GetFrame();
}

VulkanStagingBuffer::~VulkanStagingBuffer() {
	m_Device->GetMemoryMgr()->FreeBufferMemory(m_Allocation);
}

void* VulkanStagingBuffer::Map() {
	return m_Allocation.Map();
}

void VulkanStagingBuffer::Unmap() {
	m_Allocation.Unmap();
}

VkBuffer VulkanStagingBuffer::GetBuffer() {
	return m_Allocation.GetBuffer();
}

VulkanUploader::VulkanUploader(VulkanDevice* device): m_Device(device) {
}

VulkanUploader::~VulkanUploader() {
}

VulkanStagingBuffer* VulkanUploader::AcquireBuffer(uint32 bufferSize) {
	const RHIBufferDesc desc{ EBufferFlagBit::BUFFER_FLAG_COPY_SRC, bufferSize, 1};
	auto& bufferPtr =  m_StagingBuffers.EmplaceBack(new VulkanStagingBuffer(desc, m_Device));
	bufferPtr->SetName("Upload_Buffer");
	return bufferPtr.Get();
}

void VulkanUploader::BeginFrame() {
	CheckForRelease();
}

void VulkanUploader::CheckForRelease() {
	const uint32 frame = FrameCounter::GetFrame();
	for(uint32 i=0; i<m_StagingBuffers.Size(); ) {
		const uint32 createFrame = m_StagingBuffers[i]->GetCreateFrame();
		if(createFrame + RHI_MAX_FRAME_IN_FLIGHT < frame) {
			m_StagingBuffers.SwapRemoveAt(i);
		}
		else {
			++i;
		}
	}
}
