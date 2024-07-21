#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"

namespace Render {
	class Renderer {
	public:
		Renderer();
		~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer(Renderer&& rhs) noexcept;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&& rhs) noexcept;
	private:
		RHIRenderPassPtr m_RenderPas;
		RHIGraphicsPipelineStatePtr m_PSO;
	};
}