#include "Objects/Public/DirectionalLight.h"
#include "Asset/Public/AssetCommon.h"
#include "Render/Public/GlobalShader.h"
#include "Objects/Public/RenderCamera.h"
#include "System/Public/ConfigManager.h"

namespace {
	class DirectionalShadowVS : public Render::GlobalShader {
		BEGIN_SHADER_PERMUTATION
		SHADER_PERMUTATION_SWITCH(INSTANCED, false)
		END_SHADER_PERMUTATION

		BEGIN_SHADER_BINDING
		SHADER_BINDING_SET(0)
		SHADER_BINDING(0, UniformBuffer, uCamera, 1)
		SHADER_BINDING_SET(1)
		SHADER_BINDING_WITH_MACRO(0, StructuredBuffer, uInstanceData, 1, INSTANCED, true)
		SHADER_BINDING_WITH_MACRO(1, StructuredBuffer, uInstanceID, 1, INSTANCED, true)
		SHADER_BINDING_WITH_MACRO(0, UniformBuffer, uModel, 1, INSTANCED, false)
		END_SHADER_BINDING;

		GLOBAL_SHADER_IMPLEMENT(DirectionalShadowVS, "DirectionalShadow.hlsl", "MainVS", EShaderStageFlags::Vertex);
	};

	class DirectionalShadowPS : public Render::GlobalShader {
		SHADER_PERMUTATION_EMPTY();
		SHADER_BINDING_EMPTY();
		GLOBAL_SHADER_IMPLEMENT(DirectionalShadowPS, "DirectionalShadow.hlsl", "MainPS", EShaderStageFlags::Pixel);
	};
}

namespace Object {
	struct LightUBO {
		Math::FVector3 Dir; float _Padding0;
		Math::FVector4 Color;
		float FarDistances[4]; // means supports cascades up to 4
		Math::FMatrix4x4 VPMats[4];
		Math::FVector4 ShadowDebug;
	};

	inline void SplitFrustum(TArrayView<float> out, float zNear, float zFar, float logLambda) {
		// reference https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus
		// the elements of "out" means the near clip of sub frustum
		const uint32 cascadeNum = out.Size() - 1;
		CHECK(cascadeNum > 0);
		logLambda = Math::Clamp(logLambda, 0.0f, 1.0f);
		out[0] = zNear;
		for(uint32 i=1; i< cascadeNum; ++i) {
			float iDivN = (float)i / (float)cascadeNum;
			float result = 0.0f;
			if(logLambda < 1.0f) {
				float cUniform = zNear + (zFar - zNear) * iDivN;
				result += (1.0f - logLambda) * cUniform;
			}
			if(logLambda > 0.0f) {
				float cLog = zNear * Math::Pow(zFar / zNear, iDivN);
				result += logLambda * cLog;
			}
			out[i] = result;
		}
		out[cascadeNum] = zFar;
	}

	inline bool IsPSODepthBiasMatch(RHIGraphicsPipelineState* pso, float biasConst, float biasSlope) {
		if(pso) {
			const auto& rasterizer = pso->GetDesc().RasterizerState;
			return Math::FloatEqual(biasConst, rasterizer.DepthBiasConstant) && Math::FloatEqual(biasSlope, rasterizer.DepthBiasSlope);
		}
		return false;
	}

	DirectionalLight::DirectionalLight() {
		// load shadow map size
		m_ShadowConfig.ShadowMapSize = Engine::ConfigMgr::Instance().GetProjectConfig().DefaultShadowMapSize;
		for(uint32 i=0; i<CASCADE_NUM; ++i) {
			m_CascadeCameras[i].Reset(new ShadowCamera());
		}
	}

	void DirectionalLight::SetRotation(const Math::FVector3& euler) {
		Math::FQuaternion q = Math::FQuaternion::Euler(euler);
		m_LightDir = q.RotateVector3(DEFAULT_DIR);
		m_LightEuler = euler;
	}

	ShadowCamera* DirectionalLight::GetShadowCamera(uint32 i) {
		CHECK(i < CASCADE_NUM);
		return m_CascadeCameras[i].Get();
	}

	void DirectionalLight::Update(Object::RenderCamera* renderCamera) {
		// split the frustum of render camera
		UpdateCascadeSplits(renderCamera);

		// Update uniforms
		LightUBO ubo{};
		ubo.Dir = m_LightDir;
		ubo.Color = m_LightColor;
		if(GetEnableShadow()) {
			ubo.ShadowDebug.X = m_ShadowConfig.EnableDebug ? 1.0f : 0.0f;
			for (uint32 i = 0; i < CASCADE_NUM; ++i) {
				ubo.FarDistances[i] = m_FarDistances[i];
				ubo.VPMats[i] = m_CascadeCameras[i]->GetViewProjectMatrix();
				m_CascadeCameras[i]->UpdateBuffer();
			}
		}
		m_Uniform = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(ubo), &ubo, 0);

		// lazy create resources
		if (GetEnableShadow() && !IsShadowMapValid()) {
			CreateShadowMapTexture();
		}

		// check shadow bias and recreate pso
		if(GetEnableShadow()) {
			const float biasConst = m_ShadowConfig.ShadowBiasConstant, biasSlope = m_ShadowConfig.ShadowBiasSlope;
			if(!IsPSODepthBiasMatch(m_CSMRenderingPSO, biasConst, biasSlope)) {
				CreateCSMRenderingPSO();
			}
			if(!IsPSODepthBiasMatch(m_CSMInstancedRenderPSO, biasConst, biasSlope)) {
				CreateCSMInstancedRenderingPSO();
			}
		}
	}

	RHIGraphicsPipelineState* DirectionalLight::GetCSMRenderingPSO() {
		return m_CSMRenderingPSO.Get();
	}

	RHIGraphicsPipelineState* DirectionalLight::GetCSMInstancedRenderingPSO() {
		return m_CSMInstancedRenderPSO.Get();
	}

	bool DirectionalLight::IsShadowMapValid() {
		return m_ShadowMapTexture.Get() && m_ShadowMapTexture->GetDesc().Width == m_ShadowConfig.ShadowMapSize;
	}

	void DirectionalLight::CreateShadowMapTexture() {
		const ERHIFormat depthFormat = RHI::Instance()->GetDepthFormat();
		RHITextureDesc desc = RHITextureDesc::Texture2DArray(CASCADE_NUM);
		desc.Width = desc.Height = m_ShadowConfig.ShadowMapSize;
		desc.Format = depthFormat;
		desc.Flags = ETextureFlags::DepthStencilTarget | ETextureFlags::SRV;
		desc.ClearValue.DepthStencil = { 1.0f, 0u };
		m_ShadowMapTexture = RHI::Instance()->CreateTexture(desc);
	}

	void DirectionalLight::CreateCSMRenderingPSO() {
		RHIGraphicsPipelineStateDesc desc{};
		// shader
		Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
		desc.VertexShader = globalShaderMap->GetShader<DirectionalShadowVS>()->GetRHI();
		desc.PixelShader = globalShaderMap->GetShader<DirectionalShadowPS>()->GetRHI(); // TODO will report ERROR if nullptr
		// vertex input
		auto& vi = desc.VertexInput;
		vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
		vi.Attributes = { {POSITION(0), 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0} };

		desc.BlendDesc.BlendStates = { {false}, {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Null, false, m_ShadowConfig.ShadowBiasConstant, m_ShadowConfig.ShadowBiasSlope };
		desc.DepthStencilState = { true, true, ECompareType::Less, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
		desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
		desc.NumSamples = 1;
		m_CSMRenderingPSO = RHI::Instance()->CreateGraphicsPipelineState(desc);
	}

	void DirectionalLight::CreateCSMInstancedRenderingPSO() {
		RHIGraphicsPipelineStateDesc desc;
		// shader
		Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
		DirectionalShadowVS::ShaderPermutation vsp; vsp.INSTANCED = true;
		desc.VertexShader = globalShaderMap->GetShader<DirectionalShadowVS>(vsp)->GetRHI();
		DirectionalShadowPS::ShaderPermutation psp;
		desc.PixelShader = globalShaderMap->GetShader<DirectionalShadowPS>(psp)->GetRHI();
		auto& vi = desc.VertexInput;
		vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
		vi.Attributes = {
			{POSITION(0), 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},
		};

		desc.BlendDesc.BlendStates = { {false}, {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Back, false, m_ShadowConfig.ShadowBiasConstant, m_ShadowConfig.ShadowBiasSlope };
		desc.DepthStencilState = { true, true, ECompareType::Less, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
		desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
		desc.NumSamples = 1;
		m_CSMInstancedRenderPSO = RHI::Instance()->CreateGraphicsPipelineState(desc);
	}

	void DirectionalLight::UpdateCascadeSplits(Object::RenderCamera* renderCamera) {
		if (!GetEnableShadow()) {
			return;
		}
		const auto& srcView = renderCamera->GetView();
		const auto& srcProj = renderCamera->GetProjection();
		const float viewDistance = srcProj.Far - srcProj.Near;
		Object::FrustumCorner cameraFC;
		cameraFC.Build(srcView, srcProj);
		TStaticArray<float, CASCADE_NUM + 1> splits;
		SplitFrustum(splits, srcProj.Near, m_ShadowConfig.ShadowDistance, m_ShadowConfig.LogDistribution);
		for(uint32 i=0; i<CASCADE_NUM; ++i) {
			// get 8 frustum corners
			const float cascadeNear = splits[i];
			const float cascadeFar = splits[i + 1];
			const float cascadeNearRatio = (cascadeNear - srcProj.Near) / viewDistance;
			const float cascadeFarRatio = (cascadeFar - srcProj.Near) / viewDistance;
			Object::FrustumCorner cascadeFC;
			cameraFC.GetSubFrustumCorner(cascadeNearRatio, cascadeFarRatio, cascadeFC);
			// transform corners to light view space, and get min-max for aabb
			Math::FVector3 cornerCenter = cascadeFC.GetCenter();
			const Object::CameraView lightCameraView{ cornerCenter, cornerCenter + m_LightDir, {0.0f, 1.0f, 0.0f} };
			Math::FMatrix4x4 lightCameraViewMat = lightCameraView.GetViewMatrix();
			Math::FVector3 min{ FLT_MAX }, max{ -FLT_MAX };
			for(const auto& corner: cascadeFC.Corners) {
				Math::FVector3 transformedCorner = lightCameraViewMat.TransformCoord(corner);
				min = Math::FVector3::Min(transformedCorner, min);
				max = Math::FVector3::Max(transformedCorner, max);
			}
			// build projection matrix by min and max.
			const Object::CameraProjection lightCameraProj = Object::CameraProjection::Orthographic(min.X, max.X, min.Y, max.Y, min.Z, max.Z);
			m_CascadeCameras[i]->SetViewProjection(lightCameraView, lightCameraProj);
			m_FarDistances[i] = cascadeFar;
		}
	}
}
