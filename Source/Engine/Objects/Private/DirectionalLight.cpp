#include "Objects/Public/DirectionalLight.h"
#include "System/Public/EngineConfig.h"
#include "Math/Public/Math.h"
#include "Render/Public/GlobalShader.h"
#include "Render/Public/MeshRender.h"

namespace {

	IMPLEMENT_GLOBAL_SHADER(DirectionalShadowVS, "DirectionalShadow.hlsl", "MainVS", SHADER_STAGE_VERTEX_BIT);
	IMPLEMENT_GLOBAL_SHADER(DirectionalShadowPS, "DirectionalShadow.hlsl", "MainPS", SHADER_STAGE_PIXEL_BIT);

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

	DirectionalLight::DirectionalLight() {
		// create Uniforms
		for(auto& uniform: m_ShadowUniforms) {
			uniform = RHI::Instance()->CreateBuffer({ EBufferFlagBit::BUFFER_FLAG_UNIFORM, sizeof(Math::FMatrix4x4), 0 });
		}
		m_Uniform = RHI::Instance()->CreateBuffer({ EBufferFlagBit::BUFFER_FLAG_UNIFORM, sizeof(LightUBO), 0 });
	}

	void DirectionalLight::SetRotation(const Math::FVector3& euler) {
		Math::FQuaternion q = Math::FQuaternion::Euler(euler);
		m_LightDir = q.RotateVector3(DEFAULT_DIR);
		m_LightEuler = euler;
	}

	void DirectionalLight::SetEnableShadow(bool isEnable) {
		m_EnableShadow = isEnable;
	}

	void DirectionalLight::SetShadowMapSize(uint32 size) {
		m_ShadowMapSize = Math::UpperExp2(size);
		m_ShadowMapTextureDirty = true;
	}

	void DirectionalLight::SetShadowBias(float biasConst, float biasSlope) {
		if(m_ShadowMapPSO) {
			const auto& desc = m_ShadowMapPSO->GetDesc().RasterizerState;
			if(Math::FloatEqual(desc.DepthBiasConstant, biasConst) && Math::FloatEqual(desc.DepthBiasSlope, biasSlope)) {
				return;
			}
		}
		m_ShadowBiasConstant = biasConst;
		m_ShadowBiasSlope = biasSlope;
		m_ShadowMapPSODirty = true;
	}

	void DirectionalLight::GetShadowBias(float* outBiasConst, float* outBiasSlope) {
		*outBiasConst = m_ShadowBiasConstant;
		*outBiasSlope = m_ShadowBiasSlope;
	}

	Render::DrawCallQueue& DirectionalLight::GetDrawCallQueue(uint32 i) {
		CHECK(i < CASCADE_NUM);
		return m_DrawCallQueues[i];
	}

	const Math::Frustum& DirectionalLight::GetFrustum(uint32 i) {
		CHECK(i < CASCADE_NUM);
		return m_CascadeCameras[i].Frustum;
	}

	void DirectionalLight::UpdateShadowCameras(Object::RenderCamera* renderCamera) {
		// split the frustum of render camera
		const auto& srcView = renderCamera->GetView();
		const auto& srcProj = renderCamera->GetProjection();
		TStaticArray<float, CASCADE_NUM + 1> splits;
		SplitFrustum(splits, srcProj.Near, m_ShadowDistance, m_LogDistribution);
		for(uint32 i=0; i<CASCADE_NUM; ++i) {
			// get 8 frustum corners
			Object::FrustumCorner fc;
			float cascadeNear = splits[i];
			float cascadeFar = splits[i + 1];
			if(EProjType::Perspective == srcProj.ProjType) {
				fc.BuildFromPerspective(srcView, cascadeNear, cascadeFar, srcProj.Fov, srcProj.Aspect);
			}
			else {
				fc.BuildFormOrtho(srcView, cascadeNear, cascadeFar, srcProj.ViewSize, srcProj.Aspect);
			}
			// transform corners to light view space, and get min-max for aabb
			Object::CameraView dstView;
			dstView.Eye = fc.GetCenter();
			dstView.At = dstView.Eye + m_LightDir;
			dstView.Up = { 0.0f, 1.0f, 0.0f };
			auto viewMat = dstView.GetViewMatrix();
			Math::FVector3 min{ FLT_MAX }, max{ -FLT_MAX };
			for(const auto& corner: fc.Corners) {
				auto transformedCornerW = viewMat * Math::FVector4(corner.X, corner.Y, corner.Z, 1.0f);
				Math::FVector3 transformedCorner = { transformedCornerW.X, transformedCornerW.Y, transformedCornerW.Z };
				min = Math::FVector3::Min(transformedCorner, min);
				max = Math::FVector3::Max(transformedCorner, max);
			}
			Math::AABB3 aabb = Math::AABB3::MinMax(min, max);
			Math::FVector3 extent = aabb.Extent();
			// expand Z, to ensure objects locate far from camera, to avoid clip by projection
			extent.Z *= (1.4f - (float)i * 0.1f);
			Object::CameraProjection dstProj;
			dstProj.SetOrtho(-extent.Z, extent.Z, extent.Y, extent.X / extent.Y);
			// set camera
			m_CascadeCameras[i].Set(dstView, dstProj);
			m_FarDistances[i] = cascadeFar;
			m_VPMats[i] = m_CascadeCameras[i].GetViewProjectMatrix();
		}

		// Update uniforms
		LightUBO ubo;
		ubo.Dir = m_LightDir;
		ubo.Color = m_LightColor;
		for (uint32 i = 0; i < CASCADE_NUM; ++i) {
			ubo.FarDistances[i] = m_FarDistances[i];
			ubo.VPMats[i] = m_VPMats[i];
			m_ShadowUniforms[i]->UpdateData(&ubo.VPMats[i], sizeof(Math::FMatrix4x4), 0);
		}
		ubo.ShadowDebug.X = m_EnableShadowDebug ? 1.0f : 0.0f;
		m_Uniform->UpdateData(&ubo, sizeof(ubo), 0);
	}

	void DirectionalLight::UpdateShadowMapDrawCalls() {
		// lazy create resources
		if(m_ShadowMapTextureDirty) {
			CreateShadowMapTexture();
			m_ShadowMapTextureDirty = false;
		}
		if(m_ShadowMapPSODirty) {
			CreateShadowMapPSO();
			m_ShadowMapPSODirty = false;
		}
		for(uint32 i=0; i<CASCADE_NUM; ++i) {
			m_DrawCallQueues[i].Reset();
			if (m_EnableShadow) {
				m_DrawCallQueues[i].PushDrawCall([this, i](RHICommandBuffer* cmd) {
					cmd->BindGraphicsPipeline(m_ShadowMapPSO);
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_ShadowUniforms[i]));
				});
			}
		}
	}

	void DirectionalLight::CreateShadowMapTexture() {
		const ERHIFormat depthFormat = RHI::Instance()->GetDepthFormat();
		RHITextureDesc desc = RHITextureDesc::Texture2DArray(CASCADE_NUM);
		desc.Width = desc.Height = m_ShadowMapSize;
		desc.Format = depthFormat;
		desc.Flags = TEXTURE_FLAG_DEPTH_TARGET | TEXTURE_FLAG_STENCIL | TEXTURE_FLAG_SRV;
		m_ShadowMapTexture = RHI::Instance()->CreateTexture(desc);
	}

	void DirectionalLight::CreateShadowMapPSO() {
		RHIGraphicsPipelineStateDesc desc{};
		// shader
		Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
		desc.VertexShader = globalShaderMap->GetShader<DirectionalShadowVS>()->GetRHI();
		desc.PixelShader = globalShaderMap->GetShader<DirectionalShadowPS>()->GetRHI(); // TODO will report ERROR if nullptr
		// layout
		auto& layout = desc.Layout;
		layout.Resize(2);
		layout[0] = { {EBindingType::UniformBuffer, SHADER_STAGE_VERTEX_BIT} };// camera
		layout[1] = { {EBindingType::UniformBuffer, SHADER_STAGE_VERTEX_BIT} };// mesh
		// vertex input
		auto& vi = desc.VertexInput;
		vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
		vi.Attributes = {	{0, 0, ERHIFormat::R32G32B32_SFLOAT, 0} };

		desc.BlendDesc.BlendStates = { {false}, {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Null, false, m_ShadowBiasConstant, m_ShadowBiasConstant };
		desc.DepthStencilState = { true, ECompareType::Less, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
		desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
		desc.NumSamples = 1;
		m_ShadowMapPSO = RHI::Instance()->CreateGraphicsPipelineState(desc);
	}
}
