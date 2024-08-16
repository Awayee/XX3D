#pragma once
#include "Objects/Public/ECS.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/Defines.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/DrawCall.h"

namespace Object {
	class Camera;
	class DirectionalLight;

	class RenderScene: public ECSScene {
	public:
		NON_COPYABLE(RenderScene);
		NON_MOVEABLE(RenderScene);
		RenderScene();
		~RenderScene();
		Camera* GetMainCamera() { return m_Camera.Get(); }
		void Update();
		void GenerateDrawCall(Render::DrawCallQueue& queue);
		static RenderScene* GetDefaultScene(); // TODO TEST
		static void Initialize();
		static void Release();
		static void Tick();
	private:
		static TUniquePtr<RenderScene> s_Default;
		TUniquePtr<DirectionalLight> m_DirectionalLight;
		TUniquePtr<Camera> m_Camera;
		RHIBufferPtr m_UniformBuffer;
		void UpdateUniform();
		void CreateResources();
	};
}
