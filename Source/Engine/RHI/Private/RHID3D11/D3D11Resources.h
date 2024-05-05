#pragma once
#include <d3d11.h>
#include "RHI/Public/RHI.h"

namespace Engine {
	//class RImageViewD3D11RenderTarget{
	//private:
	//	ID3D11RenderTargetView* handle;
	//};

	//class RCommandBufferD3D11: public RHICommandBuffer {
	//private:
	//	bool m_IsBegin{ false };
	//public:
	//	void Begin() override { m_IsBegin = true; }
	//	void End() override { m_IsBegin = false; }
	//	void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstIndex, uint32 firstInstance) override;
	//	void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) override;
	//	void Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;
	//	void BindVertexBuffer(RBuffer* buffer, uint32 first, uint64 offset) override;
	//	void BindIndexBuffer(RBuffer* buffer, uint64 offset) override;
	//	void ClearAttachment(EImageAspectFlags aspect, const float* color, const URect& rect) override;
	//};
}
