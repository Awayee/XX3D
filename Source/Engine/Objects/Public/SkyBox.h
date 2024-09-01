#pragma once
#include "RHI/Public/RHI.h"
#include "Render/Public/DrawCall.h"
#include "Objects/Public/Camera.h"

namespace Object {
	class SkyBox {
	public:
		SkyBox(Object::RenderCamera* camera);
		~SkyBox();
		void ResetCubeMap(const XString& file);
		void CreateDrawCall(Render::DrawCallQueue& dcQueue);
	private:
		uint32 m_VertexCount{ 0 };
		uint32 m_IndexCount{ 0 };
		RHIBufferPtr m_VertexBuffer;
		RHIBufferPtr m_IndexBuffer;
		RHITexturePtr m_CubeMap;
		RHIGraphicsPipelineStatePtr m_PSO;
		Object::RenderCamera* m_Camera;

		void BuildPrimitive();
		void CreatePSO();
	};
}