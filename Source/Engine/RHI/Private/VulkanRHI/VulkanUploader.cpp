#include "VulkanUploader.h"
#include "VulkanDevice.h"

VulkanStagingBuffer::VulkanStagingBuffer(const RHIBufferDesc& desc, VulkanDevice* device) : RHIBuffer(desc), m_Device(device) {
	constexpr uint32 memoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	m_Device->GetMemoryMgr()->AllocateBufferMemory(m_Allocation, m_Desc.ByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memoryProperty);
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
	return m_StagingBuffers.EmplaceBack(new VulkanStagingBuffer(desc, m_Device)).Get();
}

void VulkanUploader::Update() {
	m_StagingBuffers.Clear();
}
