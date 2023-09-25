#pragma once
#include "RHIResources.h"
#include "Core/Public/TypeDefine.h"
#include "Core/Public/BaseStructs.h"

class RWindowHandle {
};

class RRenderPass {
public:
	virtual void SetAttachment(uint32 idx, RHITexture* texture) = 0;
	virtual void SetClearValue(uint32 idx, const RSClear& clear) = 0;
};

class RPipeline {
protected:
	RPipelineType m_Type;
public:
	RPipelineType GetType() { return m_Type; }
};

class RFramebuffer {
protected:
	USize2D m_Extent;
public:
	const USize2D& Extent() const { return m_Extent; }
};

class RDescriptorSetLayout {
	
};

class RPipelineLayout {
	
};

class RDescriptorSet {
public:
	virtual ~RDescriptorSet() = default;
	virtual void SetUniformBuffer(uint32 binding, RHIBuffer* buffer) = 0;
	virtual void SetImageSampler(uint32 binding, RHISampler* sampler, RHITexture* image) = 0;
	virtual void SetTexture(uint32 binding, RHITexture* image) = 0;
	virtual void SetSampler(uint32 binding, RHISampler* sampler) = 0;
	virtual void SetInputAttachment(uint32 binding, RHITexture* image) = 0;
};
class RSemaphore {
public:
	virtual ~RSemaphore() = default;
};