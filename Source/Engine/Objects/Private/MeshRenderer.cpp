#include "Objects/Public/MeshRenderer.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/RenderScene.h"


namespace {
	inline void FillScenePSORenderTargets(RHIGraphicsPipelineStateDesc& desc) {
		const TStaticArray<ERHIFormat, 2> colorFormats = { Object::RenderScene::GetGBufferNormalFormat(), Object::RenderScene::GetGBufferAlbedoFormat() };
		const ERHIFormat depthFormat = RHI::Instance()->GetDepthFormat();
		desc.NumColorTargets = colorFormats.Size();
		for (uint32 i = 0; i < colorFormats.Size(); ++i) {
			desc.ColorFormats[i] = colorFormats[i];
		}
		desc.DepthStencilFormat = depthFormat;
		desc.NumSamples = 1;
	}
}

namespace Object {

	void TransformData::SetTransform(const Math::FTransform& transform) {
		transform.BuildMatrix(ObjectMatrix);
		transform.BuildInverseMatrix(ObjectInvMatrix);
	}

	void TransformECSComponent::SetTransform(const Math::FTransform& transform) {
		transform.BuildMatrix(m_MatrixData.Transform);
		transform.BuildInverseMatrix(m_MatrixData.InverseTransform);
	}

	void MeshRenderSystem::Update(ECSScene* ecsScene, TransformECSComponent* transform, MeshECSComponent* meshCom) {
		// update uniform
		RHIDynamicBuffer transformBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(TransformECSComponent::MatrixData), &transform->GetMatrixData(), 0);
		// create draw call
		Object::RenderScene* scene = (Object::RenderScene*)ecsScene;
		Object::MeshRenderer2* renderer = scene->GetMeshRenderer2();
		Object::RenderCamera* camera = scene->GetMainCamera();
		Object::DirectionalLight* light = scene->GetDirectionalLight();
		for (auto& primitive : meshCom->Primitives) {
			if(INVALID_INDEX == primitive.MaterialIndex) {
				continue;
			}
			const Math::AABB3 aabb = primitive.Primitive->AABB.Transform(transform->GetMatrixData().Transform);
			if (Math::EGeometryTest::Outer != camera->GetFrustum().TestAABB(aabb)) {
				renderer->AddPrimitiveDrawData(primitive, transformBuffer);
			}
			if(light->GetEnableShadow() && meshCom->CastShadow) {
				for (uint32 i = 0; i < light->GetCascadeNum(); ++i) {
					if (Math::EGeometryTest::Outer != light->GetFrustum(i).TestAABB(aabb)) {
						light->GetMeshRenderer(i)->AddPrimitiveDrawData(primitive, transformBuffer);
					}
				}
			}
		}
	}

	void InstancedMeshRenderSystem::Update(ECSScene* ecsScene, MeshECSComponent* meshCom, InstancedDataECSComponent* instanceCom) {
		// create draw call
		Object::RenderScene* scene = (Object::RenderScene*)ecsScene;
		Object::MeshRenderer2* renderer = scene->GetMeshRenderer2();
		Object::RenderCamera* camera = scene->GetMainCamera();
		Object::DirectionalLight* light = scene->GetDirectionalLight();
		// main camera
		{
			TArray<InstanceData> instances;
			instanceCom->InstanceData.GenerateInstanceData(camera->GetFrustum(), instances);
			if (const uint32 instanceCount = instances.Size()) {
				const RHIDynamicBuffer instanceBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Vertex, instances.ByteSize(), instances.Data(), sizeof(InstanceData));
				for(auto& primitive : meshCom->Primitives) {
					if (INVALID_INDEX == primitive.MaterialIndex) {
						continue;
					}
					renderer->AdInstancedPrimitiveDrawData(primitive, instanceBuffer, instanceCount);
				}
			}
		}
		// cascade shadow
		if (light->GetEnableShadow() && meshCom->CastShadow) {
			for (uint32 i = 0; i < light->GetCascadeNum(); ++i) {
				TArray<InstanceData> instances;
				instanceCom->InstanceData.GenerateInstanceData(light->GetFrustum(i), instances);
				if (const uint32 instanceCount = instances.Size()) {
					RHIDynamicBuffer instanceBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Vertex, instances.ByteSize(), instances.Data(), sizeof(InstanceData));
					for (auto& primitive : meshCom->Primitives) {
						if (INVALID_INDEX == primitive.MaterialIndex) {
							continue;
						}
						light->GetMeshRenderer(i)->AdInstancedPrimitiveDrawData(primitive, instanceBuffer, instanceCount);
					}
				}
			}
		}
	}

	void MeshRenderer2::AddPrimitiveDrawData(const PrimitiveRenderData2& primitive, const RHIDynamicBuffer& transformBuffer) {
		const uint32 materialIndex = primitive.MaterialIndex;
		MaterialInterface* material = m_MaterialContainer->GetMaterial(materialIndex);
		if(m_BasePassPSOBatches.Size() <= materialIndex) {
			m_BasePassPSOBatches.Resize(materialIndex + 1);
		}
		auto& batch = m_BasePassPSOBatches[materialIndex];
		const uint32 psoIndex = batch.PSOIndex;
		if(INVALID_INDEX == psoIndex || m_PSOs[psoIndex].MaterialHash != material->GetHash(false)) {
			batch.PSOIndex = FindOrCreatePSO(material, false);
		}
		auto& p = m_BasePassPSOBatches[materialIndex].Primitives.EmplaceBack();
		p.Primitive = primitive.Primitive;
		p.MaterialIndex = primitive.MaterialIndex;
		p.TransformBuffer = transformBuffer;
	}

	void MeshRenderer2::AdInstancedPrimitiveDrawData(const PrimitiveRenderData2& primitive, const RHIDynamicBuffer& instanceBuffer, uint32 instanceCount) {
		const uint32 materialIndex = primitive.MaterialIndex;
		MaterialInterface* material = m_MaterialContainer->GetMaterial(materialIndex);
		if (m_BasePassInstancedBatches.Size() <= materialIndex) {
			m_BasePassInstancedBatches.Resize(materialIndex + 1);
		}
		auto& batch = m_BasePassInstancedBatches[materialIndex];
		const uint32 psoIndex = batch.PSOIndex;
		if (INVALID_INDEX == psoIndex || m_PSOs[psoIndex].MaterialHash != material->GetHash(true)) {
			batch.PSOIndex = FindOrCreatePSO(material, true);
		}
		auto& p = m_BasePassInstancedBatches[materialIndex].Primitives.EmplaceBack();
		p.Primitive = primitive.Primitive;
		p.MaterialIndex = primitive.MaterialIndex;
		p.InstanceBuffer = instanceBuffer;
		p.InstanceCount = instanceCount;
	}

	void MeshRenderer2::GenerateDrawCall(const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& basePassQueue) {
		// transforms
		{
			for (auto& batch : m_BasePassPSOBatches) {
				if(INVALID_INDEX == batch.PSOIndex) {
					continue;
				}
				RHIGraphicsPipelineState* pso = m_PSOs[batch.PSOIndex].PSO.Get();
				// bind pso
				basePassQueue.PushDrawCall([pso](RHICommandBuffer* cmd) { cmd->BindGraphicsPipeline(pso); });
				for (auto& p : batch.Primitives) {
					if(INVALID_INDEX == p.MaterialIndex) {
						continue;
					}
					const RHIDynamicBuffer transformBuffer = p.TransformBuffer;
					PrimitiveResource* primitive = p.Primitive;
					MaterialInterface* material = m_MaterialContainer->GetMaterial(p.MaterialIndex);
					basePassQueue.PushDrawCall([cameraBuffer, transformBuffer, primitive, material](RHICommandBuffer* cmd) {
						cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
						cmd->SetShaderParam(1, 0, RHIShaderParam::UniformBuffer(transformBuffer));
						material->BindBasePassShaderPrams(cmd, false);
						cmd->BindVertexBuffer(primitive->VertexBuffer.Get(), 0, 0);
						cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
						cmd->DrawIndexed(primitive->IndexCount, 1, 0, 0, 0);
					});
				}
			}
		}

		// instances
		{
			for(auto& batch: m_BasePassInstancedBatches) {
				if(INVALID_INDEX == batch.PSOIndex) {
					continue;
				}
				RHIGraphicsPipelineState* pso = m_PSOs[batch.PSOIndex].PSO.Get();
				// bind pso
				basePassQueue.PushDrawCall([pso](RHICommandBuffer* cmd) {cmd->BindGraphicsPipeline(pso); });
				for(auto& p : batch.Primitives) {
					if (INVALID_INDEX == p.MaterialIndex) {
						continue;
					}
					const RHIDynamicBuffer& instanceBuffer = p.InstanceBuffer;
					PrimitiveResource* primitive = p.Primitive;
					MaterialInterface* material = m_MaterialContainer->GetMaterial(p.MaterialIndex);
					uint32 instanceCount = p.InstanceCount;
					basePassQueue.PushDrawCall([cameraBuffer, instanceBuffer, primitive, material, instanceCount](RHICommandBuffer* cmd) {
						cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
						material->BindBasePassShaderPrams(cmd, true);
						cmd->BindVertexBuffer(primitive->VertexBuffer, 0, 0);
						cmd->BindVertexBuffer(instanceBuffer, 1, 0);
						cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
						cmd->DrawIndexed(primitive->IndexCount, instanceCount, 0, 0, 0);
					});
				}
			}
		}
	}

	void MeshRenderer2::Reset() {
		for (auto& batch : m_BasePassPSOBatches) {
			batch.Primitives.Reset();
		}
		for (auto& batch : m_BasePassInstancedBatches) {
			batch.Primitives.Reset();
		}
	}

	uint32 MeshRenderer2::FindOrCreatePSO(MaterialInterface* material, bool bInstanced) {
		if(nullptr == material) {
			return INVALID_INDEX;
		}
		const uint32 materialHs = material->GetHash(bInstanced);
		uint32 psoIndex = m_PSOs.FindIdx(materialHs);
		if(INVALID_INDEX == psoIndex) {
			psoIndex = m_PSOs.FindOrAddID(materialHs);
			RHIGraphicsPipelineStateDesc desc;
			material->FillBasePassPSODesc(desc, bInstanced);
			FillScenePSORenderTargets(desc);
			m_PSOs[psoIndex].PSO = RHI::Instance()->CreateGraphicsPipelineState(desc);
			m_PSOs[psoIndex].MaterialHash = materialHs;
		}
		return psoIndex;
	}

	void DirectionalLightMehRenderer::SetPSO(RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO) {
		m_StaticPSO = pso;
		m_StaticPSOInstanced = instancedPSO;
	}

	void DirectionalLightMehRenderer::AddPrimitiveDrawData(const PrimitiveRenderData2& primitive, const RHIDynamicBuffer& transformBuffer) {
		auto& p = m_BatchPrimitives.EmplaceBack();
		p.Primitive = primitive.Primitive;
		p.MaterialIndex = primitive.MaterialIndex;
		p.TransformBuffer = transformBuffer;
	}

	void DirectionalLightMehRenderer::AdInstancedPrimitiveDrawData(const PrimitiveRenderData2& primitive, const RHIDynamicBuffer& instanceBuffer, uint32 instanceCount) {
		auto& p = m_InstancedBatchPrimitives.EmplaceBack();
		p.Primitive = primitive.Primitive;
		p.MaterialIndex = primitive.MaterialIndex;
		p.InstanceBuffer = instanceBuffer;
		p.InstanceCount = instanceCount;
	}

	void DirectionalLightMehRenderer::GenerateDrawCall(const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& queue) {
		// transforms
		{
			queue.PushDrawCall([pso = m_StaticPSO](RHICommandBuffer* cmd) { cmd->BindGraphicsPipeline(pso); });
			for (auto& p: m_BatchPrimitives) {
				const RHIDynamicBuffer transformBuffer = p.TransformBuffer;
				PrimitiveResource* primitive = p.Primitive;
				queue.PushDrawCall([cameraBuffer, transformBuffer, primitive](RHICommandBuffer* cmd) {
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
					cmd->SetShaderParam(1, 0, RHIShaderParam::UniformBuffer(transformBuffer));
					cmd->BindVertexBuffer(primitive->VertexBuffer.Get(), 0, 0);
					cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
					cmd->DrawIndexed(primitive->IndexCount, 1, 0, 0, 0);
				});
			}
		}

		// instances
		{
			queue.PushDrawCall([pso = m_StaticPSOInstanced](RHICommandBuffer* cmd) { cmd->BindGraphicsPipeline(pso); });
			for (auto& p : m_InstancedBatchPrimitives) {
				const RHIDynamicBuffer& instanceBuffer = p.InstanceBuffer;
				PrimitiveResource* primitive = p.Primitive;
				uint32 instanceCount = p.InstanceCount;
				queue.PushDrawCall([cameraBuffer, instanceBuffer, primitive, instanceCount](RHICommandBuffer* cmd) {
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
					cmd->BindVertexBuffer(primitive->VertexBuffer, 0, 0);
					cmd->BindVertexBuffer(instanceBuffer, 1, 0);
					cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
					cmd->DrawIndexed(primitive->IndexCount, instanceCount, 0, 0, 0);
				});
			}
		}
	}

	void DirectionalLightMehRenderer::Reset() {
		m_BatchPrimitives.Reset();
		m_InstancedBatchPrimitives.Reset();
	}
}
