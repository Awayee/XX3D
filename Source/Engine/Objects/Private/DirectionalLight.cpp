#include "Objects/Public/DirectionalLight.h"
#include "System/Public/EngineConfig.h"
#include "Render/Public/GlobalShader.h"

namespace {

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
		// load shadow map size
		m_ShadowConfig.ShadowMapSize = Engine::ConfigManager::GetData().DefaultShadowMapSize;
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
		if (GetEnableShadow() && (!m_ShadowMapTexture || m_ShadowMapTexture->GetDesc().Width != m_ShadowConfig.ShadowMapSize)) {
			CreateShadowMapTexture();
		}
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

	void DirectionalLight::UpdateCascadeSplits(Object::RenderCamera* renderCamera) {
		if (!GetEnableShadow()) {
			return;
		}
		const auto& srcView = renderCamera->GetView();
		const auto& srcProj = renderCamera->GetProjection();
		TStaticArray<float, CASCADE_NUM + 1> splits;
		SplitFrustum(splits, srcProj.Near, m_ShadowConfig.ShadowDistance, m_ShadowConfig.LogDistribution);
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
				Math::FVector3 transformedCorner = viewMat.TransformCoord(corner);
				min = Math::FVector3::Min(transformedCorner, min);
				max = Math::FVector3::Max(transformedCorner, max);
			}
			Math::AABB3 aabb = Math::AABB3::MinMax(min, max);
			Math::FVector3 extent = aabb.Extent();
			// expand Z, to ensure objects locate far from camera, to avoid clip by projection
			extent.Z *= (1.4f - (float)i * 0.1f);
			// expand X, to ensure objects out of the bound can cast shadow
			extent.X *= 1.4f - (float)i * 0.05f;
			Object::CameraProjection dstProj;
			dstProj.SetOrtho(-extent.Z, extent.Z, extent.Y, extent.X / extent.Y);
			// set camera
			m_CascadeCameras[i].Set(dstView, dstProj);
			m_FarDistances[i] = cascadeFar;
			m_VPMats[i] = m_CascadeCameras[i].GetViewProjectMatrix();
		}
	}
}
