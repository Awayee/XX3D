#include "Render/Public/Renderer.h"

namespace Render {
	Renderer::Renderer() {
		m_Cmd = RHI::Instance()->CreateCommandBuffer();
	}

	Renderer::~Renderer() {
		m_Cmd.Reset();
	}

	Renderer::Renderer(Renderer&& rhs) noexcept : m_Cmd(MoveTemp(rhs.m_Cmd)) {
	}

	Renderer& Renderer::operator=(Renderer&& rhs) noexcept{
		m_Cmd = MoveTemp(rhs.m_Cmd);
		return *this;
	}

	void Renderer::Execute() {
		RHIRenderPassDesc desc;
	}

	void Renderer::ExecuteDrawCall(DrawCallQueue& queue) {
		for(auto& dc: queue) {
			dc.Execute(m_Cmd.Get());
		}
	}
}
