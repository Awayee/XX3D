#include "Objects/Public/DirectionalLight.h"
#include "Asset/Public/AssetCommon.h"
#include "System/Public/Configuration.h"
#include "Render/Public/GlobalShader.h"
#include "Objects/Public/RenderResource.h"
#include "Objects/Public/InstanceDataMgr.h"

namespace {
	class DirectionalShadowVS : public Render::GlobalShader {
		GLOBAL_SHADER_IMPLEMENT(DirectionalShadowVS, "DirectionalShadow.hlsl", "MainVS", EShaderStageFlags::Vertex);
		SHADER_PERMUTATION_BEGIN_SWITCH(INSTANCED, false);
		SHADER_PERMUTATION_END(INSTANCED);
	};

	class DirectionalShadowPS : public Render::GlobalShader {
		GLOBAL_SHADER_IMPLEMENT(DirectionalShadowPS, "DirectionalShadow.hlsl", "MainPS", EShaderStageFlags::Pixel);
		SHADER_PERMUTATION_BEGIN_SWITCH(INSTANCED, false);
		SHADER_PERMUTATION_END(INSTANCED);
	};
}

namespace Object {
	struct LightUBO {
		Math::FVector3 Dir; float _Padding0;
		Math::FVector3 Color; float _Padding1;
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
		m_ShadowConfig.ShadowMapSize = Engine::ProjectConfig::Instance().DefaultShadowMapSize;
	}

	void DirectionalLight::SetRotation(const Math::FVector3& euler) {
		Math::FQuaternion q = Math::FQuaternion::Euler(euler);
		m_LightDir = q.RotateVector3(DEFAULT_DIR);
		m_LightEuler = euler;
	}

	Render::DrawCallQueue& DirectionalLight::GetDrawCallQueue(uint32 i) {
		CHECK(i < CASCADE_NUM);
		return m_DrawCallQueues[i];
	}

	const Math::Frustum& DirectionalLight::GetFrustum(uint32 i) {
		CHECK(i < CASCADE_NUM);
		return m_CascadeCameras[i].Frustum;
	}

	void DirectionalLight::Update(Object::RenderCamera* renderCamera) {
		// clear draw call
		for (auto& queue : m_DrawCallQueues) {
			queue.Reset();
		}
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
				ubo.VPMats[i] = m_VPMats[i];
				m_ShadowUniforms[i] = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(Math::FMatrix4x4), &ubo.VPMats[i], 0);
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
		// layout
		auto& layout = desc.Layout;
		layout.Resize(2);
		layout[0] = { {EBindingType::UniformBuffer, EShaderStageFlags::Vertex} };// camera
		layout[1] = { {EBindingType::UniformBuffer, EShaderStageFlags::Vertex} };// mesh
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
		DirectionalShadowPS::ShaderPermutation psp; psp.INSTANCED = true;
		desc.PixelShader = globalShaderMap->GetShader<DirectionalShadowPS>(psp)->GetRHI();
		// layout
		desc.Layout = { { {EBindingType::UniformBuffer, EShaderStageFlags::Vertex} } };// camera
		// vertex input
		const ERHIFormat instanceRowFormat = Object::GetInstanceDataRowFormat();
		const uint32 instanceRowSize = GetRHIFormatPixelSize(instanceRowFormat);
		auto& vi = desc.VertexInput;
		vi.Bindings = {
			{0, sizeof(Asset::AssetVertex), false},
			{1, instanceRowSize * 4, true} };
		vi.Attributes = {
			{POSITION(0), 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},
			// instance transform
			{INSTANCE_TRANSFORM(0), 0, 1, instanceRowFormat, 0 },
			{INSTANCE_TRANSFORM(1), 1, 1, instanceRowFormat, instanceRowSize},
			{INSTANCE_TRANSFORM(2), 2, 1, instanceRowFormat, instanceRowSize * 2},
			{INSTANCE_TRANSFORM(3), 3, 1, instanceRowFormat, instanceRowSize * 3}, };

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
			Math::FMatrix4x4 lightCameraProjMat = lightCameraProj.GetProjectMatrix();
			m_CascadeCameras[i].Set(lightCameraView, lightCameraProj);
			m_FarDistances[i] = cascadeFar;
			m_VPMats[i] = lightCameraProjMat * lightCameraViewMat;
		}
	}
}
