#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/TArray.h"
#include "Core/Public/Func.h"

namespace Render {
	// A draw call contains pipeline, layout and vb/ib provided by scene objects.
	typedef Func<void(RHICommandBuffer*)> DrawCallFunc;

	class DrawCall {
	public:
		NON_COPYABLE(DrawCall);
		DrawCall() = default;
		~DrawCall() = default;
		DrawCall(DrawCall&& rhs) noexcept;
		DrawCall& operator=(DrawCall&& rhs) noexcept;
		void ResetFunc(DrawCallFunc&& f);
		void Execute(RHICommandBuffer* cmd);
	private:
		DrawCallFunc m_Func;
	};

	enum class EDrawCallQueueType : uint32 {
		DirectionalShadow=0,
		LocalLightShadow,
		BasePass,
		DeferredLighting,
		PostProcess,
		MaxNum,
	};

	class DrawCallQueue: public TArray<DrawCall> {
	public:
		void PushDrawCall(DrawCallFunc&& f);
		void Execute(RHICommandBuffer* cmd);
	};

	// A draw call context is obtained by cameras or renderer.
	class DrawCallContext {
	public:
		NON_COPYABLE(DrawCallContext);
		DrawCallContext() = default;
		~DrawCallContext() = default;
		DrawCallContext(DrawCallContext&& rhs) noexcept;
		DrawCallContext& operator=(DrawCallContext&& rhs)noexcept;
		void PushDrawCall(EDrawCallQueueType queueType, DrawCallFunc&& f);
		void ExecuteDraCall(EDrawCallQueueType queueType, RHICommandBuffer* cmd);
		DrawCallQueue& GetDrawCallQueue(EDrawCallQueueType queueType);
		void Reset();
	private:
		TStaticArray<DrawCallQueue, (uint32)EDrawCallQueueType::MaxNum> m_DrawCallQueues;
	};
}
