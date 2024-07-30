#include "Render/Public/RenderGraphResource.h"

namespace Render {

	void RGResourceNode::AddRef() {
		++m_RefCount;
	}

	RGBufferNode::RGBufferNode(const RHIBufferDesc& desc) : m_Desc(desc) {

	}

	RGTextureNode::RGTextureNode(const RHITextureDesc& desc) : m_Desc(desc) {
	}
}
