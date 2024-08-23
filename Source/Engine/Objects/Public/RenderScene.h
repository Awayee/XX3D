#pragma once
#include "Objects/Public/ECS.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/Defines.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/Renderer.h"

namespace Object {
	class Camera;
	class DirectionalLight;

	class RenderScene: public ECSScene, public Render::IRenderScene {
	public:
		NON_COPYABLE(RenderScene);
		NON_MOVEABLE(RenderScene);
		RenderScene();
		~RenderScene();
		Camera* GetMainCamera() { return m_Camera.Get(); }
		DirectionalLight* GetDirectionalLight() { return m_DirectionalLight.Get(); }
		Render::DrawCallContext& GetDrawCallContext() { return m_DrawCallContext; }
		void Update();
		void SetViewportSize(const USize2D& size);
		void SetRenderTarget(RHITexture* target, RHITextureSubDesc sub);
		Render::RGTextureNode* Render(Render::RenderGraph& rg) override;
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
		RHIBufferPtr m_CameraUniform;
		RHIBufferPtr m_LightUniform;
		Rect m_Viewport;
		Render::DrawCallContext m_DrawCallContext;
		USize2D m_ViewportSize;
		RHITexturePtr m_DirectionalShadowMap;
		RHITexturePtr m_GBufferNormal;
		RHITexturePtr m_GBufferAlbedo;
		RHITexturePtr m_Depth;
		RHITexture* m_RenderTarget;
		RHITextureSubDesc m_RenderTargetSub;
		RHIGraphicsPipelineStatePtr MeshGBufferPSO;
		RHIGraphicsPipelineStatePtr m_DeferredLightingPSO;
		bool m_ViewportDirty;
		bool m_RenderTargetDirty;
		void UpdateSceneDrawCall();
		void CreateResources();
		void CreateTextures();
	};
}
