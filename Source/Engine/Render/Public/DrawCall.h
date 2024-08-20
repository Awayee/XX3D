#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/TArray.h"
#include "Core/Public/Func.h"

namespace Render {
	// A draw call contains pipeline, layout and vb/ib provided by scene objects.
	class DrawCall {
	public:
		NON_COPYABLE(DrawCall);
		DrawCall() = default;
		~DrawCall() = default;
		DrawCall(DrawCall&& rhs) noexcept;
		DrawCall& operator=(DrawCall&& rhs) noexcept;
		void ResetFunc(Func<void(RHICommandBuffer*)>&& f);
		void Execute(RHICommandBuffer* cmd);
	private:
		Func<void(RHICommandBuffer*)> m_Func;
	};

	enum class EDrawCallQueueType : uint32 {
		DirectionalShadow=0,
		BasePass,
		DeferredLighting,
		PostProcess,
		MaxNum,
	};

	typedef TArray<DrawCall> DrawCallQueue;

	// A draw call context is obtained by cameras or renderer.
	class DrawCallContext {
	public:
		NON_COPYABLE(DrawCallContext);
		DrawCallContext() = default;
		~DrawCallContext() = default;
		DrawCallContext(DrawCallContext&& rhs) noexcept;
		DrawCallContext& operator=(DrawCallContext&& rhs)noexcept;
		void PushDrawCall(EDrawCallQueueType queueType, Func<void(RHICommandBuffer*)>&& f);
		DrawCallQueue& GetDrawCallQueue(EDrawCallQueueType queueType);
		void Reset();
	private:
		TStaticArray<DrawCallQueue, (uint32)EDrawCallQueueType::MaxNum> m_DrawCallQueues;
	};
}
