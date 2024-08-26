#pragma once
#include "RHI/Public/RHI.h"
#include "Render/Public/DrawCall.h"
#include "Objects/Public/Camera.h"

namespace Object {
	class DirectionalLight {
	public:
		DirectionalLight();
		void SetRotation(const Math::FVector3& euler); // rotation relative to DEFAULT_DIR;
		const Math::FVector3& GetRotation()const { return m_LightEuler; }
		void SetColor(const Math::FVector3& color) { m_LightColor = color; }
		const Math::FVector3& GetColor() const { return m_LightColor; }

		// for shadow map
		void SetEnableShadow(bool isEnable);
		bool GetEnableShadow() const { return m_EnableShadow; }
		void SetEnableShadowDebug(bool isEnable) { m_EnableShadowDebug = isEnable; }
		bool GetEnableShadowDebug()const { return m_EnableShadowDebug; }
		void SetShadowMapSize(uint32 size);
		uint32 GetShadowMapSize()const { return m_ShadowMapSize; }
		void SetShadowDistance(float distance) { m_ShadowDistance = distance; }
		float GetShadowDistance()const { return m_ShadowDistance; }
		float GetLogDistribution() const { return m_LogDistribution; }
		void SetLogDistribution(float distribution) { m_LogDistribution = distribution; }
		void SetShadowBias(float biasConst, float biasSlope);
		void GetShadowBias(float* outBiasConst, float* outBiasSlope);

		RHITexture* GetShadowMap() { return m_ShadowMapTexture.Get(); }
		Render::DrawCallQueue& GetDrawCallQueue(uint32 i);
		const Math::Frustum& GetFrustum(uint32 i);
		void UpdateShadowCameras(Object::RenderCamera* renderCamera);
		void UpdateShadowMapDrawCalls();
		static uint32 GetCascadeNum() { return CASCADE_NUM; }
		// for scene rendering
		RHIBuffer* GetUniform() { return m_Uniform.Get(); }
	private:
		// for scene render
		static constexpr uint32 CASCADE_NUM = 4;
		inline static const Math::FVector3 DEFAULT_DIR {0, 0, 1};
		Math::FVector3 m_LightDir;
		Math::FVector3 m_LightEuler;
		Math::FVector3 m_LightColor{0.7f,0.8f,0.8f};
		RHIBufferPtr m_Uniform;

		// for shadow
		bool m_EnableShadow{ true };
		bool m_EnableShadowDebug{ false };
		float m_ShadowDistance{ 128.0f };
		float m_LogDistribution{ 0.9f };
		uint32 m_ShadowMapSize{ 1024 };
		float m_ShadowBiasConstant{ 6.0f };
		float m_ShadowBiasSlope{ 6.0f };
		RHITexturePtr m_ShadowMapTexture; // lazy create
		RHIGraphicsPipelineStatePtr m_ShadowMapPSO;// lazy create
		TStaticArray<Object::Camera, CASCADE_NUM> m_CascadeCameras;
		TStaticArray<Render::DrawCallQueue, CASCADE_NUM> m_DrawCallQueues;
		TStaticArray<float, CASCADE_NUM> m_FarDistances;
		TStaticArray<Math::FMatrix4x4, CASCADE_NUM> m_VPMats;
		TStaticArray<RHIBufferPtr, CASCADE_NUM> m_ShadowUniforms;// for shadow map
		bool m_ShadowMapTextureDirty{ true };
		bool m_ShadowMapPSODirty{ true };

		void CreateShadowMapTexture();
		void CreateShadowMapPSO();
	};
}