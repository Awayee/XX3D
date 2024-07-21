#pragma once
#include "VulkanCommon.h"
#include "VulkanMemory.h"

class VulkanDevice;

class VulkanStagingBuffer: public RHIBuffer {
public:
	VulkanStagingBuffer(const RHIBufferDesc& desc, VulkanDevice* device);
	~VulkanStagingBuffer() override;
	void UpdateData(const void* data, uint32 byteSize) override { ASSERT(0, ""); }
	void* Map();
	void Unmap();
	VkBuffer GetBuffer();

private:
	VulkanDevice* m_Device;
	BufferAllocation m_Allocation;
};

class VulkanUploader {
public:
	explicit VulkanUploader(VulkanDevice* device);
	~VulkanUploader();
	VulkanStagingBuffer* AcquireBuffer(uint32 bufferSize);// return a staging buffer, which will be released after cmd executing.
	void Update();
private:
	VulkanDevice* m_Device;
	TVector<TUniquePtr<VulkanStagingBuffer>> m_StagingBuffers;// Buffers to upload in current frame.
};