#pragma once
#include "Objects/Public/ECS.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/Defines.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/Renderer.h"
#include "Render/Public/HZBBuilder.h"

namespace Object {
	class RenderCamera;
	class DirectionalLight;
	class PrimitiveMgr;

	class RenderScene: public ECSScene{
	public:
		static void AddConstructFunc(XFunc<void, RenderScene*>&& f);
		NON_COPYABLE(RenderScene);
		NON_MOVEABLE(RenderScene);
		RenderScene();
		~RenderScene();
		RenderCamera* GetMainCamera() { return m_Camera.Get(); }
		DirectionalLight* GetDirectionalLight() { return m_DirectionalLight.Get(); }
		PrimitiveMgr* GetPrimitiveMgr() { return m_PrimitiveMgr.Get(); }
		Render::DrawCallQueue& GetLightingPassDrawCallQueue() { return m_LightingPassDrawCallQueue; }
		void Update();
		void Render(Render::RenderGraph& rg, Render::RGTextureNode* targetNode);
		inline static ERHIFormat GetGBufferNormalFormat() { return ERHIFormat::R8G8B8A8_UNORM; }
		inline static ERHIFormat GetGBufferAlbedoFormat() { return ERHIFormat::R8G8B8A8_UNORM; }
		static RenderScene* GetDefaultScene(); // TODO TEST
		static void Initialize();
		static void Release();
		static void Tick();
	private:
		static TUniquePtr<RenderScene> s_Default;
		TUniquePtr<DirectionalLight> m_DirectionalLight;
		TUniquePtr<RenderCamera> m_Camera;
		TUniquePtr<PrimitiveMgr> m_PrimitiveMgr;
		Render::DrawCallQueue m_LightingPassDrawCallQueue;
		// fo gBuffer
		USize2D m_TargetSize;
		RHITexturePtr m_GBufferNormal;
		RHITexturePtr m_GBufferAlbedo;
		RHITexturePtr m_Depth;
		RHITexturePtr m_DepthSRV;
		// for deferred lighting
		TUniquePtr<RHIGraphicsPipelineState> m_DeferredLightingPSO;
		// for hzb
		Render::HZBBuilder m_HzbBuilder;

		void CreateDeferredLightingDrawCall();
		void CreateTextureResources();
	};
#define RENDER_SCENE_REGISTER_SYSTEM(cls) private:\
	inline static const uint8 s_RegisterSystem = (Object::RenderScene::AddConstructFunc([](RenderScene* scene){scene->RegisterSystem<cls>();}), 0)
}
