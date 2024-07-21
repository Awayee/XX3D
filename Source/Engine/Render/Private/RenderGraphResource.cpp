#include "Render/Public/RenderGraphResource.h"

namespace Render {

	void RGResource::AddRef() {
		++m_RefCount;
	}

	RGBuffer::RGBuffer(const RHIBufferDesc& desc) : m_Desc(desc) {

	}

	RGBuffer::RGBuffer(RHIBufferDesc&& desc): m_Desc(std::forward<RHIBufferDesc>(desc)) {
	}
}
