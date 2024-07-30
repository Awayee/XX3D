#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"
#include "Render/Public/DrawCallMgr.h"
namespace Render {
	class Renderer {
	public:
		NON_COPYABLE(Renderer);
		Renderer();
		~Renderer();
		Renderer(Renderer&& rhs) noexcept;
		Renderer& operator=(Renderer&& rhs) noexcept;
		void SetRenderTarget();
		void Execute();
	private:
		void ExecuteDrawCall(DrawCallQueue& queue);
		RHICommandBufferPtr m_Cmd;
	};
}