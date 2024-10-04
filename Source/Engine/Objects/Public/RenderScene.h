#pragma once
#include "Objects/Public/ECS.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/Defines.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/Renderer.h"

namespace Object {
	class RenderCamera;
	class DirectionalLight;

	class RenderScene: public ECSScene{
	public:
		NON_COPYABLE(RenderScene);
		NON_MOVEABLE(RenderScene);
		RenderScene();
		~RenderScene();
		RenderCamera* GetMainCamera() { return m_Camera.Get(); }
		DirectionalLight* GetDirectionalLight() { return m_DirectionalLight.Get(); }
		Render::DrawCallContext& GetDrawCallContext() { return m_DrawCallContext; }
		void Update();
		void Render(Render::RenderGraph& rg, Render::RGTextureNode* targetNode);
		static RenderScene* GetDefaultScene(); // TODO TEST
		static void Initialize();
		static void Release();
		static void Tick();
	private:
		static TUniquePtr<RenderScene> s_Default;
		TUniquePtr<DirectionalLight> m_DirectionalLight;
		TUniquePtr<RenderCamera> m_Camera;
		Render::DrawCallContext m_DrawCallContext;
		// fo gBuffer
		USize2D m_TargetSize;
		RHITexturePtr m_GBufferNormal;
		RHITexturePtr m_GBufferAlbedo;
		RHITexturePtr m_Depth;
		RHITexturePtr m_DepthSRV;
		RHIGraphicsPipelineStatePtr m_GBufferPSO;
		// for deferred lighting
		RHIGraphicsPipelineStatePtr m_DeferredLightingPSO;

		void UpdateSceneDrawCall();
		void CreateResources();
		void CreateTextureResources();
		void CreatePSOs();
	};

	void RenderSceneAddConstructFunc(Func<void(RenderScene*)>&& f);

	template<class T>
	void RenderSceneRegisterSystem() {
		RenderSceneAddConstructFunc([](RenderScene* scene) {scene->RegisterSystem<T>(); });
	}
#define RENDER_SCENE_REGISTER_SYSTEM(cls) private: inline static const uint8 s_RegisterSystem = (Object::RenderSceneRegisterSystem<cls>(), 0)
}
