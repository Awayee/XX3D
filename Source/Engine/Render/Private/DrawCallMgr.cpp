#include "Render/Public/DrawCallMgr.h"

namespace Render {
	DrawCall::DrawCall() {
	}

	DrawCall::~DrawCall() {
	}

	DrawCall::DrawCall(DrawCall&& rhs) noexcept: m_Funcs(MoveTemp(rhs.m_Funcs)) {
	}

	DrawCall& DrawCall::operator=(DrawCall&& rhs) noexcept {
		m_Funcs = MoveTemp(rhs.m_Funcs);
		return *this;
	}

	void DrawCall::Execute(RHICommandBuffer* cmd) {
		for (auto& f: m_Funcs) {
			f(cmd);
		}
	}
}
