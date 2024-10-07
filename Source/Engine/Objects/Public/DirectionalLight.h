#pragma once
#include "RHI/Public/RHI.h"
#include "Render/Public/DrawCall.h"
#include "Objects/Public/Camera.h"

namespace Object {

	struct DirectionalShadowConfig {
		bool EnableShadow{ true };
		bool EnableDebug{ false };
		uint32 ShadowMapSize{ 1024 };
		float ShadowDistance{ 128.0f };
		float LogDistribution{ 0.9f };
		float ShadowBiasConstant{ 3.0f };
		float ShadowBiasSlope{ 3.0f };
	};

	class DirectionalLight {
	public:
		DirectionalLight();
		void SetRotation(const Math::FVector3& euler); // rotation relative to DEFAULT_DIR;
		const Math::FVector3& GetRotation()const { return m_LightEuler; }
		void SetColor(const Math::FVector3& color) { m_LightColor = color; }
		const Math::FVector3& GetColor() const { return m_LightColor; }

		// for shadow map
		const DirectionalShadowConfig& GetShadowConfig() const { return m_ShadowConfig; }
		void SetShadowConfig(const DirectionalShadowConfig& config) { m_ShadowConfig = config; }
		bool GetEnableShadow() const { return m_ShadowConfig.EnableShadow; }
		RHITexture* GetShadowMap() { return m_ShadowMapTexture.Get(); }
		Render::DrawCallQueue& GetDrawCallQueue(uint32 i);
		const Math::Frustum& GetFrustum(uint32 i);
		void Update(Object::RenderCamera* renderCamera);
		static uint32 GetCascadeNum() { return CASCADE_NUM; }
		// for scene rendering
		const RHIDynamicBuffer& GetLightingUniform() { return m_Uniform; }
		const RHIDynamicBuffer& GetShadowUniform(uint32 cascade) { return m_ShadowUniforms[cascade]; }
	private:
		// for scene render
		static constexpr uint32 CASCADE_NUM = 4;
		inline static const Math::FVector3 DEFAULT_DIR {0, 0, 1};
		Math::FVector3 m_LightDir;
		Math::FVector3 m_LightEuler;
		Math::FVector3 m_LightColor{0.7f,0.8f,0.8f};
		RHIDynamicBuffer m_Uniform;

		// for shadow
		DirectionalShadowConfig m_ShadowConfig;
		RHITexturePtr m_ShadowMapTexture; // lazy create
		RHIGraphicsPipelineStatePtr m_ShadowMapPSO;// lazy create
		TStaticArray<Object::Camera, CASCADE_NUM> m_CascadeCameras;
		TStaticArray<Render::DrawCallQueue, CASCADE_NUM> m_DrawCallQueues;
		TStaticArray<float, CASCADE_NUM> m_FarDistances;
		TStaticArray<Math::FMatrix4x4, CASCADE_NUM> m_VPMats;
		TStaticArray<RHIDynamicBuffer, CASCADE_NUM> m_ShadowUniforms;// for shadow map

		void CreateShadowMapTexture();
	};
}