#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/TVector.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TSingleton.h"
#include "Core/Public/Func.h"

namespace Render {
	class DrawCall {
	public:
		NON_COPYABLE(DrawCall);
		DrawCall();
		~DrawCall();
		DrawCall(DrawCall&& rhs) noexcept;
		DrawCall& operator=(DrawCall&& rhs) noexcept;
		void Execute(RHICommandBuffer* cmd);
	private:
		TVector<Func<void(RHICommandBuffer*)>> m_Funcs;
	};

	struct EDrawCallQueueType { enum {
		SHADOW_PASS,
		BASE_PASS,
		POST_PROCESS,
		COUNT,
	};};

	typedef TVector<DrawCall> DrawCallQueue;

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
