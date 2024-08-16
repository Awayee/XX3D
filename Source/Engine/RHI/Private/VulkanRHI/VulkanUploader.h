#pragma once
#include "VulkanCommon.h"
#include "VulkanMemory.h"

class VulkanDevice;
class VulkanCommandBuffer;

class VulkanStagingBuffer: public RHIBuffer {
public:
	VulkanStagingBuffer(const RHIBufferDesc& desc, VulkanDevice* device);
	~VulkanStagingBuffer() override;
	void UpdateData(const void* data, uint32 byteSize, uint32 offset) override { ASSERT(0, ""); }
	void* Map();
	void Unmap();
	VkBuffer GetBuffer();
	uint32 GetCreateFrame() const { return m_CreateFrame; }

private:
	VulkanDevice* m_Device;
	BufferAllocation m_Allocation;
	uint32 m_CreateFrame;
};

class VulkanUploader {
public:
	explicit VulkanUploader(VulkanDevice* device);
	~VulkanUploader();
	VulkanStagingBuffer* AcquireBuffer(uint32 bufferSize);// return a staging buffer, which will be released after cmd executing.
	void BeginFrame();
private:
	VulkanDevice* m_Device;
	TArray<TUniquePtr<VulkanStagingBuffer>> m_StagingBuffers;// Buffers to upload in current frame.
	void CheckForRelease();
};