#pragma once
#include "Objects/Public/ECS.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/Defines.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/Renderer.h"

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
		Render::DrawCallContext& GetDrawCallContext() { return m_DrawCallContext; }
		void Update();
		static RenderScene* GetDefaultScene(); // TODO TEST
		static void Initialize();
		static void Release();
		static void Tick();
		template<class T> static void RegisterSystemStatic() {
			s_RegisterSystems.PushBack([](RenderScene* scene) { scene->RegisterSystem<T>(); });
		}
	private:
		static TUniquePtr<RenderScene> s_Default;
		static TArray<Func<void(RenderScene*)>> s_RegisterSystems;
		TUniquePtr<DirectionalLight> m_DirectionalLight;
		TUniquePtr<Camera> m_Camera;
		RHIBufferPtr m_UniformBuffer;
		Render::DrawCallContext m_DrawCallContext;
		Render::ViewportRenderer m_Renderer;// TODO separate from scene
		void ResetSceneDrawCall();
		void CreateResources();
	};
}
