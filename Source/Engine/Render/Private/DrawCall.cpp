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

	DrawCallContext::DrawCallContext(DrawCallContext&& rhs) noexcept {
		for(uint32 i=0; i<(uint32)EDrawCallQueueType::MaxNum; ++i) {
			m_DrawCallQueues[i].Swap(rhs.m_DrawCallQueues[i]);
		}
	}

	DrawCallContext& DrawCallContext::operator=(DrawCallContext&& rhs) noexcept {
		for (uint32 i = 0; i < (uint32)EDrawCallQueueType::MaxNum; ++i) {
			m_DrawCallQueues[i].Swap(rhs.m_DrawCallQueues[i]);
		}
		return *this;
	}

	void DrawCallContext::PushDrawCall(EDrawCallQueueType queueType, Func<void(RHICommandBuffer*)>&& f) {
		m_DrawCallQueues[(uint32)queueType].EmplaceBack().ResetFunc(MoveTemp(f));
	}

	DrawCallQueue& DrawCallContext::GetDrawCallQueue(EDrawCallQueueType queueType) {
		return m_DrawCallQueues[(uint32)queueType];
	}

	void DrawCallContext::Reset() {
		for(auto& queue : m_DrawCallQueues) {
			queue.Reset();
		}
	}
}
