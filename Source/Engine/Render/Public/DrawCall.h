#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/TVector.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TSingleton.h"
#include "Core/Public/Func.h"

namespace Render {
	// A draw call contains pipeline, layout and vb/ib provided by scene objects.
	class DrawCall {
	public:
		NON_COPYABLE(DrawCall);
		DrawCall() = default;
		~DrawCall();
		DrawCall(DrawCall&& rhs) noexcept;
		DrawCall& operator=(DrawCall&& rhs) noexcept;
		void ResetFunc(Func<void(RHICommandBuffer*)>&& f);
		void Execute(RHICommandBuffer* cmd);
	private:
		Func<void(RHICommandBuffer*)> m_Func;
	};

	struct EDrawCallQueueType { enum {
		SHADOW_PASS,
		BASE_PASS,
		DEFERRED_LIGHTING_PASS,
		POST_PROCESS,
		COUNT,
	};};

	typedef TVector<DrawCall> DrawCallQueue;

	// A draw call context is obtained by cameras or renderer.
	class DrawCallContext {
	public:
		NON_COPYABLE(DrawCallContext);
		NON_MOVEABLE(DrawCallContext);
		DrawCallContext();
		~DrawCallContext();
	private:
		TStaticArray<DrawCallQueue, EDrawCallQueueType::COUNT> m_DrawCallQueues;
	};

	class DrawCallMgr {
	public:
		DrawCallMgr();
		~DrawCallMgr();
		DrawCallContext& GetDrawCallContext();
	private:
		friend TSingleton<DrawCallMgr>;
		DrawCallContext m_Context;
	};
}
