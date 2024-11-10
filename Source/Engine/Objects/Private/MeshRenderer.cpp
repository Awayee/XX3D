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

	void ObjectTransformData::SetTransform(const Math::FTransform& transform) {
		transform.BuildMatrix(ObjectMatrix);
		transform.BuildInverseMatrix(ObjectInvMatrix);
	}

	void TransformECSComponent::SetTransform(const Math::FTransform& transform) {
		Transform = transform;
		TransformData.SetTransform(transform);
	}

	void MeshRenderSystem:: Update(ECSScene* ecsScene, TransformECSComponent* transform, MeshECSComponent* meshCom) {
		PrimitiveRenderer* primitiveRenderer = ((RenderScene*)ecsScene)->GetPrimitiveRenderer();
		primitiveRenderer->AddPrimitive(transform->TransformData, meshCom->Primitives, meshCom->RenderFlags);
	}

	void InstancedMeshRenderSystem::Update(ECSScene* ecsScene, MeshECSComponent* meshCom, InstancedDataECSComponent* instanceCom) {
		// create draw call
		PrimitiveRenderer* primitiveRenderer = ((RenderScene*)ecsScene)->GetPrimitiveRenderer();
		primitiveRenderer->AddInstancedPrimitive(&instanceCom->InstanceData, meshCom->Primitives, meshCom->RenderFlags);
	}

	uint32 MaterialPSOCache::FindOrAddMaterial(MaterialInterface* material) {
		if(auto iter = m_MapMaterialIndex.find(material); iter!=m_MapMaterialIndex.end()) {
			return iter->second;
		}
		uint32 materialIndex = m_MaterialCaches.Size();
		m_MaterialCaches.PushBack({ material, 0, {INVALID_INDEX, INVALID_INDEX} });
		m_MapMaterialIndex.try_emplace(material, materialIndex);
		return materialIndex;
	}

	MaterialInterface* MaterialPSOCache::GetMaterial(uint32 materialIndex) const {
		if(materialIndex < m_MaterialCaches.Size()) {
			return m_MaterialCaches[materialIndex].Material;
		}
		return nullptr;
	}

	uint32 MaterialPSOCache::GetPSOIndex(uint32 materialIndex, bool bInstanced) {
		MaterialCache& materialCache = m_MaterialCaches[materialIndex];
		return m_MaterialCaches[materialIndex].PSOIndex[bInstanced];
	}

	RHIGraphicsPipelineState* MaterialPSOCache::GetPSO(uint32 materialIndex, bool bInstanced) {
		auto& materialCache = m_MaterialCaches[materialIndex];
		++materialCache.RefCounter;
		uint32& indexRef = materialCache.PSOIndex[bInstanced];
		const uint32 hs = materialCache.Material->GetHash(bInstanced);
		if(INVALID_INDEX == indexRef || indexRef >= m_PSOCaches.Size() || m_PSOCaches[indexRef].MaterialHash != hs) {
			indexRef = m_PSOCaches.FindIdx(hs);
			if(INVALID_INDEX == indexRef) {
				indexRef = m_PSOCaches.FindOrAddID(hs);
				RHIGraphicsPipelineStateDesc desc;
				materialCache.Material->FillBasePassPSODesc(desc, bInstanced);
				FillScenePSORenderTargets(desc);
				m_PSOCaches[indexRef].PSO = RHI::Instance()->CreateGraphicsPipelineState(desc);
				m_PSOCaches[indexRef].MaterialHash = hs;
			}
		}
		auto& psoCache = m_PSOCaches[indexRef];
		++psoCache.RefCounter;
		return psoCache.PSO.Get();
	}

	void MaterialPSOCache::Reset() {
		m_MaterialCaches.Reset();
		m_MapMaterialIndex.clear();
		m_PSOCaches.Reset();
	}

	void MaterialPSOCache::Clean() {
		for(uint32 i=0; i<m_MaterialCaches.Size();) {
			if(m_MaterialCaches[i].RefCounter == 0) {
				m_MapMaterialIndex.erase(m_MaterialCaches[i].Material);
				if(i != m_MaterialCaches.Size() - 1) {
					m_MapMaterialIndex[m_MaterialCaches.Back().Material] = i;
				}
				m_MaterialCaches.SwapRemoveAt(i);
			}
			else {
				++i;
			}
		}
		// clean objects with no references
		for(auto& materialCache: m_MaterialCaches) {
			materialCache.RefCounter = 0;
		}
		for(auto& psoCache: m_PSOCaches) {
			psoCache.RefCounter = 0;
		}
	}

	void PrimitiveRendererCPUDriven::AddPrimitive(const ObjectTransformData& transform, TArrayView<PrimitiveRenderData2> primitives, TEnumFlag<ERenderPassType> passFlag) {
		const uint32 transformIndex = m_Transforms.Size();
		m_Transforms.PushBack(transform);
		for(PrimitiveRenderData2& primitive: primitives) {
			CorrectPrimitiveMaterialIndex(&primitive);
			PrimitiveCache& primitiveCache = m_PrimitiveCaches.EmplaceBack();
			primitiveCache.Primitive = primitive.Primitive;
			primitiveCache.MaterialIndex = primitive.MaterialIndex;
			primitiveCache.TransformIndex = transformIndex;
			const Math::AABB3 aabb = primitive.Primitive->AABB.Transform(transform.ObjectMatrix);
			m_PrimitiveAABBs.PushBack(aabb);
		}
		CHECK(m_PrimitiveAABBs.Size() == m_PrimitiveCaches.Size());
	}

	void PrimitiveRendererCPUDriven::AddInstancedPrimitive(InstanceDataMgr* instanceDataMgr, TArrayView<PrimitiveRenderData2> primitives, TEnumFlag<ERenderPassType> passFlag) {
		const uint32 instanceDataIndex = m_InstanceDataMgrs.Size();
		m_InstanceDataMgrs.PushBack(instanceDataMgr);
		for(PrimitiveRenderData2& primitive: primitives) {
			CorrectPrimitiveMaterialIndex(&primitive);
			InstancedPrimitiveCache& primitiveCache = m_InstancedPrimitiveCaches.EmplaceBack();
			primitiveCache.Primitive = primitive.Primitive;
			primitiveCache.MaterialIndex = primitive.MaterialIndex;
			primitiveCache.InstanceDataIndex = instanceDataIndex;
		}
	}

	void PrimitiveRendererCPUDriven::GenerateBasePassDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer,
		Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue) {
		PrimitiveCullResult result;
		FrustumCull(frustum, result);
		// transforms
		for(uint32 primitiveIndex: result.Primitives) {
			const PrimitiveCache& cache = m_PrimitiveCaches[primitiveIndex];
			const RHIDynamicBuffer& transformBuffer = result.TransformBuffers[cache.TransformIndex];
			CHECK(transformBuffer.IsValid());
			RHIGraphicsPipelineState* pso = m_MaterialPSOCache->GetPSO(cache.MaterialIndex, false);
			CHECK(pso);
			MaterialInterface* material = m_MaterialPSOCache->GetMaterial(cache.MaterialIndex);
			PrimitiveResource* primitive = cache.Primitive;
			renderingQueue.PushDrawCall([pso, transformBuffer, material, primitive, cameraBuffer](RHICommandBuffer* cmd) {
				cmd->BindGraphicsPipeline(pso);
				cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
				cmd->SetShaderParam(1, 0, RHIShaderParam::UniformBuffer(transformBuffer));
				material->BindBasePassShaderPrams(cmd, false);
				cmd->BindVertexBuffer(primitive->VertexBuffer.Get(), 0, 0);
				cmd->BindIndexBuffer(primitive->IndexBuffer.Get(), 0);
				cmd->DrawIndexed(primitive->IndexCount, 1, 0, 0, 0);
			});
		}

		// instances
		for(auto[primitiveIndex, instanceCount]: result.InstancedPrimitives) {
			const InstancedPrimitiveCache& cache = m_InstancedPrimitiveCaches[primitiveIndex];
			RHIDynamicBuffer& instanceBuffer = result.InstanceBuffers[cache.InstanceDataIndex];
			MaterialInterface* material = m_MaterialPSOCache->GetMaterial(cache.MaterialIndex);
			RHIGraphicsPipelineState* pso = m_MaterialPSOCache->GetPSO(cache.MaterialIndex, true);
			PrimitiveResource* primitive = cache.Primitive;
			renderingQueue.PushDrawCall([pso, cameraBuffer, instanceBuffer, primitive, material, instanceCount=instanceCount](RHICommandBuffer* cmd) {
				cmd->BindGraphicsPipeline(pso);
				cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
				material->BindBasePassShaderPrams(cmd, true);
				cmd->BindVertexBuffer(primitive->VertexBuffer, 0, 0);
				cmd->BindVertexBuffer(instanceBuffer, 1, 0);
				cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
				cmd->DrawIndexed(primitive->IndexCount, instanceCount, 0, 0, 0);
			});
		}
	}

	void PrimitiveRendererCPUDriven::GenerateDirectionalShadowDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer,
		Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue,
		RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO){
		PrimitiveCullResult result;
		FrustumCull(frustum, result);
		// transforms
		if(result.Primitives.Size()) {
			renderingQueue.PushDrawCall([pso](RHICommandBuffer* cmd) {cmd->BindGraphicsPipeline(pso); });
			for (uint32 primitiveIndex : result.Primitives) {
				const PrimitiveCache& cache = m_PrimitiveCaches[primitiveIndex];
				const RHIDynamicBuffer& transformBuffer = result.TransformBuffers[cache.TransformIndex];
				CHECK(transformBuffer.IsValid());
				PrimitiveResource* primitive = cache.Primitive;
				renderingQueue.PushDrawCall([transformBuffer, primitive, cameraBuffer](RHICommandBuffer* cmd) {
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
					cmd->SetShaderParam(1, 0, RHIShaderParam::UniformBuffer(transformBuffer));
					cmd->BindVertexBuffer(primitive->VertexBuffer.Get(), 0, 0);
					cmd->BindIndexBuffer(primitive->IndexBuffer.Get(), 0);
					cmd->DrawIndexed(primitive->IndexCount, 1, 0, 0, 0);
				});
			}
		}

		// instances
		if(result.InstancedPrimitives.Size()) {
			renderingQueue.PushDrawCall([instancedPSO](RHICommandBuffer* cmd) {cmd->BindGraphicsPipeline(instancedPSO); });
			for (auto [primitiveIndex, instanceCount] : result.InstancedPrimitives) {
				const InstancedPrimitiveCache& cache = m_InstancedPrimitiveCaches[primitiveIndex];
				RHIDynamicBuffer& instanceBuffer = result.InstanceBuffers[cache.InstanceDataIndex];
				PrimitiveResource* primitive = cache.Primitive;
				renderingQueue.PushDrawCall([cameraBuffer, instanceBuffer, primitive, instanceCount = instanceCount](RHICommandBuffer* cmd) {
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
					cmd->BindVertexBuffer(primitive->VertexBuffer, 0, 0);
					cmd->BindVertexBuffer(instanceBuffer, 1, 0);
					cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
					cmd->DrawIndexed(primitive->IndexCount, instanceCount, 0, 0, 0);
				});
			}
		}
	}

	void PrimitiveRendererCPUDriven::Clean() {
		m_Transforms.Reset();
		m_PrimitiveAABBs.Reset();
		m_PrimitiveCaches.Reset();
		m_InstanceDataMgrs.Reset();
		m_InstancedPrimitiveCaches.Reset();
		m_MaterialPSOCache->Clean();
	}

	void PrimitiveRendererCPUDriven::CorrectPrimitiveMaterialIndex(PrimitiveRenderData2* primitive) {
		uint32& materialIndexRef = primitive->MaterialIndex;
		// never added, or retired, new
		if (INVALID_INDEX == materialIndexRef ||
			m_MaterialPSOCache->GetMaterial(materialIndexRef) != primitive->Material) {
			materialIndexRef = m_MaterialPSOCache->FindOrAddMaterial(primitive->Material);
		}
	}

	void PrimitiveRendererCPUDriven::FrustumCull(const Math::Frustum& frustum, PrimitiveCullResult& outResult) {
		outResult.Primitives.Reserve(m_PrimitiveCaches.Size());
		outResult.TransformBuffers.Resize(m_Transforms.Size());
		const uint32 primitiveCount = m_PrimitiveCaches.Size();
		for (uint32 i = 0; i < primitiveCount; ++i) {
			const PrimitiveCache& cache = m_PrimitiveCaches[i];
			const Math::AABB3& aabb = m_PrimitiveAABBs[i];
			if (frustum.TestAABBSimple(aabb)) {
				RHIDynamicBuffer& transformBuffer = outResult.TransformBuffers[cache.TransformIndex];
				if (!transformBuffer.IsValid()) {
					const ObjectTransformData& transformData = m_Transforms[cache.TransformIndex];
					transformBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(transformData), &transformData, 0);
				}
				outResult.Primitives.PushBack(i);
			}
		}

		outResult.InstancedPrimitives.Reserve(m_InstancedPrimitiveCaches.Size());
		outResult.InstanceBuffers.Resize(m_InstanceDataMgrs.Size());
		TArray<uint32> instanceCounts(m_InstanceDataMgrs.Size(), UINT32_MAX);
		for (uint32 i = 0; i < m_InstancedPrimitiveCaches.Size(); ++i) {
			const InstancedPrimitiveCache& cache = m_InstancedPrimitiveCaches[i];
			const uint32 instanceDataIndex = cache.InstanceDataIndex;
			if (UINT32_MAX == instanceCounts[instanceDataIndex]) {
				InstanceDataMgr* instanceDataMgr = m_InstanceDataMgrs[instanceDataIndex];
				TArray<InstanceData> instances;
				instanceDataMgr->GenerateInstanceData(frustum, instances);
				instanceCounts[instanceDataIndex] = instances.Size();
				outResult.InstanceBuffers[instanceDataIndex] = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Vertex, instances.ByteSize(), instances.Data(), sizeof(InstanceData));
			}
			outResult.InstancedPrimitives.PushBack({ i, instanceCounts[instanceDataIndex] });
		}
	}
}
