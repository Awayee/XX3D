#include "Render/Public/RenderGraphResource.h"

namespace Engine {

	void RGResource::AddRef() {
		++m_RefCount;
	}

	RGBuffer::RGBuffer(const Desc& desc) {

	}

	RGBuffer::RGBuffer(Desc&& desc): RGBuffer(desc) {
	}
}
