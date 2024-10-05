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

	class DrawCallQueue: public TArray<DrawCall> {
	public:
		void PushDrawCall(DrawCallFunc&& f);
		void Execute(RHICommandBuffer* cmd);
	};
}
