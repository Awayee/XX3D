#include "Render/Public/DrawCall.h"

namespace Render {
	DrawCall::DrawCall(DrawCall&& rhs) noexcept: m_Func(MoveTemp(rhs.m_Func)) {
	}

	DrawCall& DrawCall::operator=(DrawCall&& rhs) noexcept {
		m_Func = MoveTemp(rhs.m_Func);
		return *this;
	}

	void DrawCall::ResetFunc(Func<void(RHICommandBuffer*)>&& f) {
		m_Func = MoveTemp(f);
	}

	void DrawCall::Execute(RHICommandBuffer* cmd) {
		if(m_Func) {
			m_Func(cmd);
		}
	}

	void DrawCallQueue::PushDrawCall(DrawCallFunc&& f) {
		EmplaceBack().ResetFunc(MoveTemp(f));
	}

	void DrawCallQueue::Execute(RHICommandBuffer* cmd) {
		for(uint32 i=0; i<Size(); ++i) {
			operator[](i).Execute(cmd);
		}
	}
}
