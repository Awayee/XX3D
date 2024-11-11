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

	void MeshRenderSystem::Update(ECSScene* ecsScene, TransformECSComponent* transform, MeshECSComponent* meshCom) {
		PrimitiveMgr* primitiveMgr = ((RenderScene*)ecsScene)->GetPrimitiveMgr();
		primitiveMgr->AddPrimitive(transform->TransformData, meshCom->Primitives, meshCom->RenderFlags);
	}

	void InstancedMeshRenderSystem::Update(ECSScene* ecsScene, MeshECSComponent* meshCom, InstancedDataECSComponent* instanceCom) {
		// create draw call
		PrimitiveMgr* primitiveMgr = ((RenderScene*)ecsScene)->GetPrimitiveMgr();
		primitiveMgr->AddInstancedPrimitive(&instanceCom->InstanceData, meshCom->Primitives, meshCom->RenderFlags);
	}

	uint32 PrimitiveMaterialPSOCache::FindOrAddMaterial(MaterialInterface* material) {
		if (auto iter = m_MapMaterialIndex.find(material); iter != m_MapMaterialIndex.end()) {
			return iter->second;
		}
		uint32 materialIndex = m_MaterialCaches.Size();
		m_MaterialCaches.PushBack({ material, 0, {INVALID_INDEX, INVALID_INDEX} });
		m_MapMaterialIndex.try_emplace(material, materialIndex);
		return materialIndex;
	}

	MaterialInterface* PrimitiveMaterialPSOCache::GetMaterial(uint32 materialIndex) const {
		if(materialIndex < m_MaterialCaches.Size()) {
			return m_MaterialCaches[materialIndex].Material;
		}
		return nullptr;
	}

	RHIGraphicsPipelineState* PrimitiveMaterialPSOCache::GetPSO(uint32 materialIndex, bool bInstanced) {
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
		return psoCache.PSO.Get();
	}

	uint32 PrimitiveMaterialPSOCache::GetMaterialSize() const {
		return m_MaterialCaches.Size();
	}

	void PrimitiveMaterialPSOCache::Reset() {
		m_MaterialCaches.Reset();
		m_MapMaterialIndex.clear();
		m_PSOCaches.Reset();
	}

	void PrimitiveMaterialPSOCache::Clean() {
		// Clean material and pso with no reference
		for (uint32 i = 0; i < m_MaterialCaches.Size();) {
			if (m_MaterialCaches[i].RefCounter == 0) {
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

		// Reset ref counter
		for (auto& materialCache : m_MaterialCaches) {
			materialCache.RefCounter = 0;
		}
	}

	PrimitiveMgr::PrimitiveMgr() {
		m_MaterialPSOCache.Reset(new PrimitiveMaterialPSOCache);
		m_PrimitiveRenderer.Reset(new PrimitiveRendererCPUDriven(m_MaterialPSOCache.Get()));
		m_PrimitiveInstancedRenderer.Reset(new PrimitiveInstancedRendererCPUDriven(m_MaterialPSOCache.Get()));
	}

	void PrimitiveMgr::AddPrimitive(const ObjectTransformData& transform, TArrayView<PrimitiveRenderData> primitives, TEnumFlag<ERenderPassType> passFlag) {
		if (!primitives.Size() || !passFlag.Value) {
			return;
		}
		for (auto& primitive : primitives) {
			CorrectPrimitiveMaterialIndex(&primitive);
		}
		m_PrimitiveRenderer->Add(transform, primitives, passFlag);
	}

	void PrimitiveMgr::AddInstancedPrimitive(InstanceDataMgr* instanceDataMgr, TArrayView<PrimitiveRenderData> primitives, TEnumFlag<ERenderPassType> passFlag) {
		if (!primitives.Size() || !passFlag.Value) {
			return;
		}
		for (auto& primitive : primitives) {
			CorrectPrimitiveMaterialIndex(&primitive);
		}
		m_PrimitiveInstancedRenderer->Add(instanceDataMgr, primitives, passFlag);
	}

	void PrimitiveMgr::GenerateBasePassDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue) {
		m_PrimitiveRenderer->GenerateBasePassDrawCall(frustum, cameraBuffer, cullingQueue, renderingQueue);
		m_PrimitiveInstancedRenderer->GenerateBasePassDrawCall(frustum, cameraBuffer, cullingQueue, renderingQueue);
	}

	void PrimitiveMgr::GenerateDirectionalShadowDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue, RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO) {
		m_PrimitiveRenderer->GenerateDirectionalShadowDrawCall(frustum, cameraBuffer, cullingQueue, renderingQueue, pso, instancedPSO);
		m_PrimitiveInstancedRenderer->GenerateDirectionalShadowDrawCall(frustum, cameraBuffer, cullingQueue, renderingQueue, pso, instancedPSO);
	}

	void PrimitiveMgr::Clean() {
		m_PrimitiveRenderer->Reset();
		m_PrimitiveInstancedRenderer->Reset();
		m_MaterialPSOCache->Clean();
	}

	void PrimitiveMgr::CorrectPrimitiveMaterialIndex(PrimitiveRenderData* primitive) {
		uint32& materialIndexRef = primitive->MaterialIndex;
		// never added, or retired, new
		if (INVALID_INDEX == materialIndexRef ||
			m_MaterialPSOCache->GetMaterial(materialIndexRef) != primitive->Material) {
			materialIndexRef = m_MaterialPSOCache->FindOrAddMaterial(primitive->Material);
		}
	}

	void PrimitiveRendererCPUDriven::Add(const ObjectTransformData& transform, TArrayView<PrimitiveRenderData> primitives, TEnumFlag<ERenderPassType> passFlag) {
		const uint32 transformIndex = m_Transforms.Size();
		m_Transforms.PushBack(transform);
		for(PrimitiveRenderData& primitive: primitives) {
			uint32 aabbIndex = m_PrimitiveAABBs.Size();
			const Math::AABB3 aabb = primitive.Primitive->AABB.Transform(transform.ObjectMatrix);
			m_PrimitiveAABBs.PushBack(aabb);
			for (uint32 i = 0; i < EnumCast(ERenderPassType::MaxNum); ++i) {
				if (passFlag.HasFlag((ERenderPassType)i)) {
					PrimitiveCache& primitiveCache = m_PrimitiveCacheArrays[i].EmplaceBack();
					primitiveCache.Primitive = primitive.Primitive;
					primitiveCache.MaterialIndex = primitive.MaterialIndex;
					primitiveCache.TransformIndex = transformIndex;
					primitiveCache.AABBIndex = aabbIndex;
				}
			}
		}
	}

	void PrimitiveRendererCPUDriven::Reset() {
		m_Transforms.Reset();
		m_PrimitiveAABBs.Reset();
		for(auto& primitivesCaches : m_PrimitiveCacheArrays) {
			primitivesCaches.Reset();
		}
	}

	void PrimitiveRendererCPUDriven::GenerateBasePassDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue) {
		PrimitiveCullResult result;
		FrustumCull(frustum, result, ERenderPassType::BasePass);
		// transforms
		const auto& primitiveCaches = m_PrimitiveCacheArrays[EnumCast(ERenderPassType::BasePass)];
		for(uint32 primitiveIndex : result.Primitives) {
			const PrimitiveCache& cache = primitiveCaches[primitiveIndex];
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
	}

	void PrimitiveRendererCPUDriven::GenerateDirectionalShadowDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue, RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO) {
		PrimitiveCullResult result;
		FrustumCull(frustum, result, ERenderPassType::DirectionalShadow);
		// transforms
		const auto& primitiveCaches = m_PrimitiveCacheArrays[EnumCast(ERenderPassType::DirectionalShadow)];
		if(result.Primitives.Size()) {
			renderingQueue.PushDrawCall([pso](RHICommandBuffer* cmd) {cmd->BindGraphicsPipeline(pso); });
			for (uint32 primitiveIndex : result.Primitives) {
				const PrimitiveCache& cache = primitiveCaches[primitiveIndex];
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
	}

	void PrimitiveRendererCPUDriven::FrustumCull(const Math::Frustum& frustum, PrimitiveCullResult& outResult, ERenderPassType pass) {
		const auto& primitiveCaches = m_PrimitiveCacheArrays[EnumCast(pass)];
		outResult.Primitives.Reserve(primitiveCaches.Size());
		outResult.TransformBuffers.Resize(m_Transforms.Size());
		const uint32 primitiveCount = primitiveCaches.Size();
		for (uint32 i = 0; i < primitiveCount; ++i) {
			const PrimitiveCache& cache = primitiveCaches[i];
			const Math::AABB3& aabb = m_PrimitiveAABBs[cache.AABBIndex];
			if (frustum.TestAABBSimple(aabb)) {
				RHIDynamicBuffer& transformBuffer = outResult.TransformBuffers[cache.TransformIndex];
				if (!transformBuffer.IsValid()) {
					const ObjectTransformData& transformData = m_Transforms[cache.TransformIndex];
					transformBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(transformData), &transformData, 0);
				}
				outResult.Primitives.PushBack(i);
			}
		}
	}

	void PrimitiveInstancedRendererCPUDriven::Add(InstanceDataMgr* instanceDataMgr, TArrayView<PrimitiveRenderData> primitives, TEnumFlag<ERenderPassType> passFlag) {
		const uint32 instanceDataIndex = m_InstanceDataMgrs.Size();
		m_InstanceDataMgrs.PushBack(instanceDataMgr);
		for (PrimitiveRenderData& primitive : primitives) {
			for (uint32 i = 0; i < EnumCast(ERenderPassType::MaxNum); ++i) {
				if (passFlag.HasFlag((ERenderPassType)i)) {
					InstancedPrimitiveCache& primitiveCache = m_InstancedPrimitiveCacheArrays[i].EmplaceBack();
					primitiveCache.Primitive = primitive.Primitive;
					primitiveCache.MaterialIndex = primitive.MaterialIndex;
					primitiveCache.InstanceDataIndex = instanceDataIndex;
				}
			}
		}
	}

	void PrimitiveInstancedRendererCPUDriven::Reset() {
		m_InstanceDataMgrs.Reset();
		for (auto& primitiveCaches : m_InstancedPrimitiveCacheArrays) {
			primitiveCaches.Reset();
		}
	}

	void PrimitiveInstancedRendererCPUDriven::GenerateBasePassDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue) {
		PrimitiveCullResult result;
		FrustumCull(frustum, result, ERenderPassType::BasePass);
		// instances
		const auto& instancePrimitiveCaches = m_InstancedPrimitiveCacheArrays[EnumCast(ERenderPassType::BasePass)];
		for (auto [primitiveIndex, instanceCount] : result.InstancedPrimitives) {
			const InstancedPrimitiveCache& cache = instancePrimitiveCaches[primitiveIndex];
			RHIDynamicBuffer& instanceBuffer = result.InstanceBuffers[cache.InstanceDataIndex];
			MaterialInterface* material = m_MaterialPSOCache->GetMaterial(cache.MaterialIndex);
			RHIGraphicsPipelineState* pso = m_MaterialPSOCache->GetPSO(cache.MaterialIndex, true);
			PrimitiveResource* primitive = cache.Primitive;
			renderingQueue.PushDrawCall([pso, cameraBuffer, instanceBuffer, primitive, material, instanceCount = instanceCount](RHICommandBuffer* cmd) {
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

	void PrimitiveInstancedRendererCPUDriven::GenerateDirectionalShadowDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue, RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO) {
		PrimitiveCullResult result;
		FrustumCull(frustum, result, ERenderPassType::DirectionalShadow);
		// instances
		const auto& instancedPrimitiveCaches = m_InstancedPrimitiveCacheArrays[EnumCast(ERenderPassType::DirectionalShadow)];
		if (result.InstancedPrimitives.Size()) {
			renderingQueue.PushDrawCall([instancedPSO](RHICommandBuffer* cmd) {cmd->BindGraphicsPipeline(instancedPSO); });
			for (auto [primitiveIndex, instanceCount] : result.InstancedPrimitives) {
				const InstancedPrimitiveCache& cache = instancedPrimitiveCaches[primitiveIndex];
				RHIDynamicBuffer& instanceBuffer = result.InstanceBuffers[cache.InstanceDataIndex];
				PrimitiveResource* primitive = cache.Primitive;
				renderingQueue.PushDrawCall([cameraBuffer, instanceBuffer, primitive, instanceCount = instanceCount](RHICommandBuffer* cmd) {
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
					cmd->BindVertexBuffer(primitive->VertexBuffer, 0, 0);
					cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
					cmd->BindVertexBuffer(instanceBuffer, 1, 0);
					cmd->DrawIndexed(primitive->IndexCount, instanceCount, 0, 0, 0);
				});
			}
		}
	}

	void PrimitiveInstancedRendererCPUDriven::FrustumCull(const Math::Frustum& frustum, PrimitiveCullResult& outResult, ERenderPassType pass) {
		const auto& instancedPrimitiveCaches = m_InstancedPrimitiveCacheArrays[EnumCast(pass)];
		outResult.InstancedPrimitives.Reserve(instancedPrimitiveCaches.Size());
		outResult.InstanceBuffers.Resize(m_InstanceDataMgrs.Size());
		TArray<uint32> instanceCounts(m_InstanceDataMgrs.Size(), UINT32_MAX);
		for (uint32 i = 0; i < instancedPrimitiveCaches.Size(); ++i) {
			const InstancedPrimitiveCache& cache = instancedPrimitiveCaches[i];
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
