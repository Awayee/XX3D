#include "Objects/Public/MeshRenderer.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/RenderCamera.h"
#include "Objects/Public/RenderScene.h"
#include "Render/public/DefaultResource.h"
#include "Render/Public/GlobalShader.h"
#include "System/Public/Configuration.h"

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

	class InstanceCullingCS: public Render::GlobalShader {
		GLOBAL_SHADER_IMPLEMENT(InstanceCullingCS, "InstanceCulling.hlsl", "MainCS", EShaderStageFlags::Compute);
		SHADER_PERMUTATION_BEGIN_SWITCH(OCCLUSION_TEST, false);
		SHADER_PERMUTATION_END(OCCLUSION_TEST);
	};

	inline bool CheckMeshComponentValid(Object::MeshECSComponent* com) {
		return com->Primitives.Size();
	}
}

namespace Object {

	void ObjectTransformData::SetTransform(const Math::FTransform& transform) {
		transform.BuildMatrix(ObjectMatrix);
		transform.BuildInverseMatrix(ObjectInvMatrix);
	}

	void TransformECSComponent::SetTransform(const Math::FTransform& transform) {
		TransformData.SetTransform(transform);
	}

	void MeshRenderSystem::Update(ECSScene* ecsScene, TransformECSComponent* transform, MeshECSComponent* meshCom) {
		if(CheckMeshComponentValid(meshCom)) {
			PrimitiveMgr* primitiveMgr = ((RenderScene*)ecsScene)->GetPrimitiveMgr();
			primitiveMgr->AddPrimitive(meshCom, transform);
		}
	}

	void InstancedMeshRenderSystem::Update(ECSScene* ecsScene, MeshECSComponent* meshCom, InstancedDataECSComponent* instanceCom) {
		if(CheckMeshComponentValid(meshCom)) {
			PrimitiveMgr* primitiveMgr = ((RenderScene*)ecsScene)->GetPrimitiveMgr();
			primitiveMgr->AddInstancedPrimitive(meshCom, instanceCom);
		}
	}

	RHIGraphicsPipelineState* PrimitiveMaterialPSOCache::GetPSO(MaterialChild* handle, bool bInstanced) {
		// find or add
		const uint32 idx = FixMaterialIndex(handle);
		handle->MaterialIndex = idx;
		return FindOrAddPSO(idx, bInstanced);
	}

	void PrimitiveMaterialPSOCache::AddMaterialRef(MaterialChild* handle) {
		const uint32 idx = FixMaterialIndex(handle);
		++m_MaterialCaches[idx].RefCounter;
		handle->MaterialIndex = idx;
	}

	void PrimitiveMaterialPSOCache::RemoveMaterialRef(MaterialChild* handle) {
		const uint32 idx = FixMaterialIndex(handle);
		if(m_MaterialCaches[idx].RefCounter) {
			++m_MaterialCaches[idx].RefCounter;
		}
		handle->MaterialIndex = INVALID_INDEX;
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

		// Reset ref counter
		for(auto& materialCache: m_MaterialCaches) {
			materialCache.RefCounter = 0;
		}
	}

	uint32 PrimitiveMaterialPSOCache::FixMaterialIndex(const MaterialChild* handle) {
		uint32 idx = handle->MaterialIndex;
		if (INVALID_INDEX == idx || idx >= m_MaterialCaches.Size() || m_MaterialCaches[idx].Material != handle->Material) {
			idx = FindOrAddMaterial(handle->Material);
		}
		return idx;
	}

	uint32 PrimitiveMaterialPSOCache::FindOrAddMaterial(MaterialInterface* material) {
		uint32 materialIndex;
		if (auto iter = m_MapMaterialIndex.find(material); iter != m_MapMaterialIndex.end()) {
			materialIndex = iter->second;
		}
		else {
			materialIndex = m_MaterialCaches.Size();
			m_MaterialCaches.PushBack({ material, 0, {INVALID_INDEX, INVALID_INDEX} });
			m_MapMaterialIndex.try_emplace(material, materialIndex);
		}
		++m_MaterialCaches[materialIndex].RefCounter;
		return materialIndex;
	}

	MaterialInterface* PrimitiveMaterialPSOCache::GetMaterial(uint32 materialIndex) const {
		return m_MaterialCaches[materialIndex].Material;
	}

	RHIGraphicsPipelineState* PrimitiveMaterialPSOCache::FindOrAddPSO(uint32 materialIndex, bool bInstanced) {
		auto& materialCache = m_MaterialCaches[materialIndex];
		++materialCache.RefCounter;
		uint32& indexRef = materialCache.PSOIndex[bInstanced];
		const uint32 hs = materialCache.Material->GetHash(bInstanced);
		if (INVALID_INDEX == indexRef || indexRef >= m_PSOCaches.Size() || m_PSOCaches[indexRef].MaterialHash != hs) {
			indexRef = m_PSOCaches.FindIdx(hs);
			if (INVALID_INDEX == indexRef) {
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

	PrimitiveMgr::PrimitiveMgr() {
		m_MaterialPSOCache.Reset(new PrimitiveMaterialPSOCache);
		m_PrimitiveRenderer.Reset(new PrimitiveRendererCPUDriven(m_MaterialPSOCache.Get()));
		const bool enableGPUDriven = Engine::ProjectConfig::Instance().EnableGPUDriven;
		if(enableGPUDriven) {
			m_PrimitiveInstancedRenderer.Reset(new PrimitiveInstancedRendererGPUDriven(m_MaterialPSOCache.Get()));
		}
		else {
			m_PrimitiveInstancedRenderer.Reset(new PrimitiveInstancedRendererCPUDriven(m_MaterialPSOCache.Get()));
		}
	}

	void PrimitiveMgr::AddPrimitive(MeshECSComponent* mesh, TransformECSComponent* transform) {
		m_PrimitiveRenderer->Add(mesh, transform);
	}

	void PrimitiveMgr::AddInstancedPrimitive(MeshECSComponent* mesh, InstancedDataECSComponent* instanceData) {
		m_PrimitiveInstancedRenderer->Add(mesh, instanceData);
	}

	void PrimitiveMgr::PreDrawCall() {
		m_PrimitiveRenderer->PreDrawCall();
		m_PrimitiveInstancedRenderer->PreDrawCall();
	}

	void PrimitiveMgr::GenerateBasePassDrawCall(Object::RenderCamera* camera) {
		m_PrimitiveRenderer->GenerateBasePassDrawCall(camera);
		m_PrimitiveInstancedRenderer->GenerateBasePassDrawCall(camera);
	}

	void PrimitiveMgr::GenerateDirectionalShadowDrawCall(Object::ShadowCamera* camera, RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO) {
		m_PrimitiveRenderer->GenerateDirectionalShadowDrawCall(camera, pso);
		m_PrimitiveInstancedRenderer->GenerateDirectionalShadowDrawCall(camera, instancedPSO);
	}

	void PrimitiveMgr::Clean() {
		m_PrimitiveRenderer->Reset();
		m_PrimitiveInstancedRenderer->Reset();
		m_MaterialPSOCache->Clean();
	}

	void PrimitiveRendererCPUDriven::Add(MeshECSComponent* mesh, TransformECSComponent* transform) {
		uint32& indexRef = transform->CacheIndex;
		if(indexRef == INVALID_INDEX) {
			indexRef = m_PrimitiveStorage.Allocate();
			PrimitiveCacheGroup& cacheGroup = m_PrimitiveStorage.Get(indexRef);
			cacheGroup.PrimitiveCaches.Reserve(mesh->Primitives.Size());
			for(PrimitiveRenderData& primitiveData: mesh->Primitives) {
				PrimitiveCache& cache = cacheGroup.PrimitiveCaches.EmplaceBack();
				cache.Material = primitiveData.Material;
				cache.MaterialIndex = INVALID_INDEX;
				cache.Primitive = primitiveData.Primitive;
				m_MaterialPSOCache->AddMaterialRef(&cache);
			}
		}
		PrimitiveCacheGroup& cacheGroup = m_PrimitiveStorage.Get(indexRef);
		// check transform and update AABB
		if(cacheGroup.Transform.ObjectMatrix != transform->TransformData.ObjectMatrix) {
			cacheGroup.Transform = transform->TransformData;
			for(auto& cache: cacheGroup.PrimitiveCaches) {
				cache.AABB = cache.Primitive->AABB.Transform(transform->TransformData.ObjectMatrix);
			}
		}
		++cacheGroup.RefCount;
		for(uint32 i=0; i<EnumCast(ERenderPassType::MaxNum); ++i) {
			if(mesh->RenderFlags.HasFlag((ERenderPassType)i)) {
				m_RenderingCacheArray[i].PushBack(indexRef);
			}
		}
	}

	void PrimitiveRendererCPUDriven::Reset() {
		for(auto& renderingCache: m_RenderingCacheArray) {
			renderingCache.Reset();
		}
		// Clear primitives with no ref
		for(uint32 i=0; i<m_PrimitiveStorage.Size(); ++i) {
			PrimitiveCacheGroup& cacheGroup = m_PrimitiveStorage.Get(i);
			if(cacheGroup.IsValid()) {
				cacheGroup.BufferIndex = INVALID_INDEX;
				if(0 == cacheGroup.RefCount) {
					for(PrimitiveCache& cache: cacheGroup.PrimitiveCaches) {
						m_MaterialPSOCache->RemoveMaterialRef(&cache);
					}
					m_PrimitiveStorage.Free(i);
				}
				cacheGroup.RefCount = 0;
			}
		}
		m_TransformBuffers.Reset();
		m_TransformBuffers.Reserve(m_PrimitiveStorage.Size());
	}

	void PrimitiveRendererCPUDriven::GenerateBasePassDrawCall(Object::RenderCamera* camera) {
		Render::DrawCallQueue& queue = camera->GetRenderContext().RenderingQueue;
		const Math::Frustum& frustum = camera->GetFrustum();
		const auto& renderingIndices = m_RenderingCacheArray[EnumCast(ERenderPassType::BasePass)];
		for(uint32 i : renderingIndices) {
			PrimitiveCacheGroup& cacheGroup = m_PrimitiveStorage.Get(i);
			for(auto& cache: cacheGroup.PrimitiveCaches) {
				if(frustum.TestAABBSimple(cache.AABB)) {
					const RHIDynamicBuffer& transformBuffer = GetTransformBuffer(i);
					CHECK(transformBuffer.IsValid());
					MaterialInterface* material = cache.Material;
					RHIGraphicsPipelineState* pso = m_MaterialPSOCache->GetPSO(&cache, false);
					CHECK(pso);
					queue.PushDrawCall([pso, transformBuffer, material, primitive = cache.Primitive, cameraBuffer = camera->GetBuffer()](RHICommandBuffer* cmd) {
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
		}
	}

	void PrimitiveRendererCPUDriven::GenerateDirectionalShadowDrawCall(Object::ShadowCamera* camera, RHIGraphicsPipelineState* pso) {
		Render::DrawCallQueue& queue = camera->GetRenderContext().RenderingQueue;
		const Math::Frustum& frustum = camera->GetFrustum();
		const auto& renderingIndices = m_RenderingCacheArray[EnumCast(ERenderPassType::DirectionalShadow)];
		for(uint32 i : renderingIndices) {
			PrimitiveCacheGroup& cacheGroup = m_PrimitiveStorage.Get(i);
			for(auto& cache: cacheGroup.PrimitiveCaches) {
				if(frustum.TestAABBSimple(cache.AABB)) {
					const RHIDynamicBuffer& transformBuffer = GetTransformBuffer(i);
					CHECK(transformBuffer.IsValid());
					queue.PushDrawCall([pso, transformBuffer, primitive = cache.Primitive, cameraBuffer = camera->GetBuffer()](RHICommandBuffer* cmd) {
						cmd->BindGraphicsPipeline(pso);
						cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
						cmd->SetShaderParam(1, 0, RHIShaderParam::UniformBuffer(transformBuffer));
						cmd->BindVertexBuffer(primitive->VertexBuffer.Get(), 0, 0);
						cmd->BindIndexBuffer(primitive->IndexBuffer.Get(), 0);
						cmd->DrawIndexed(primitive->IndexCount, 1, 0, 0, 0);
					});
				}
			}
		}
	}

	const RHIDynamicBuffer& PrimitiveRendererCPUDriven::GetTransformBuffer(uint32 groupIndex) {
		CHECK(groupIndex < m_PrimitiveStorage.Size());
		PrimitiveCacheGroup& group = m_PrimitiveStorage.Get(groupIndex);
		if(INVALID_INDEX == group.BufferIndex) {
			group.BufferIndex = m_TransformBuffers.Size();
			m_TransformBuffers.PushBack(RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(group.Transform), &group.Transform, 0));
		}
		return m_TransformBuffers[group.BufferIndex];
	}

	void PrimitiveInstancedRendererCPUDriven::Add(MeshECSComponent* mesh, InstancedDataECSComponent* instanceData) {
		uint32& indexRef = instanceData->CacheIndex;
		if (indexRef == INVALID_INDEX) {
			indexRef = m_PrimitiveStorage.Allocate();
			InstancedPrimitiveCacheGroup& cacheGroup = m_PrimitiveStorage.Get(indexRef);
			cacheGroup.DataMgr = &instanceData->InstanceData;
			cacheGroup.PrimitiveCaches.Reserve(mesh->Primitives.Size());
			for (PrimitiveRenderData& primitiveData : mesh->Primitives) {
				InstancedPrimitiveCache& cache = cacheGroup.PrimitiveCaches.EmplaceBack();
				cache.Material = primitiveData.Material;
				cache.MaterialIndex = INVALID_INDEX;
				cache.Primitive = primitiveData.Primitive;
				m_MaterialPSOCache->AddMaterialRef(&cache);
			}
		}
		++m_PrimitiveStorage.Get(indexRef).RefCount;
		for (uint32 i = 0; i < EnumCast(ERenderPassType::MaxNum); ++i) {
			if (mesh->RenderFlags.HasFlag((ERenderPassType)i)) {
				m_RenderingCacheArray[i].PushBack(indexRef);
			}
		}
	}

	void PrimitiveInstancedRendererCPUDriven::Reset() {
		for(auto& renderingCache: m_RenderingCacheArray) {
			renderingCache.Reset();
		}
		for(uint32 i=0; i<m_PrimitiveStorage.Size(); ++i) {
			InstancedPrimitiveCacheGroup& group = m_PrimitiveStorage.Get(i);
			if(group.IsValid()) {
				if(0 == group.RefCount) {
					for(InstancedPrimitiveCache& cache: group.PrimitiveCaches) {
						m_MaterialPSOCache->RemoveMaterialRef(&cache);
					}
					m_PrimitiveStorage.Free(i);
				}
				group.RefCount = 0;
			}
		}
	}

	void PrimitiveInstancedRendererCPUDriven::GenerateBasePassDrawCall(Object::RenderCamera* camera) {
		Render::DrawCallQueue& queue = camera->GetRenderContext().RenderingQueue;
		const Math::Frustum& frustum = camera->GetFrustum();
		const auto& renderingIndices = m_RenderingCacheArray[EnumCast(ERenderPassType::BasePass)];
		for(uint32 i : renderingIndices) {
			InstancedPrimitiveCacheGroup& group = m_PrimitiveStorage.Get(i);
			TArray<uint32> instanceIDs;
			group.DataMgr->GenerateInstanceID(frustum, instanceIDs);
			if(instanceIDs.IsEmpty()) {
				continue;
			}
			RHIDynamicBuffer instanceIDBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::SRV,
				instanceIDs.ByteSize(), instanceIDs.Data(), sizeof(uint32));
			RHIBuffer* instanceBuffer = group.DataMgr->GetInstanceBuffer();
			uint32 instanceCount = instanceIDs.Size();
			for(auto& cache: group.PrimitiveCaches) {
				MaterialInterface* material = cache.Material;
				RHIGraphicsPipelineState* pso = m_MaterialPSOCache->GetPSO(&cache, true);
				queue.PushDrawCall([pso, instanceBuffer, instanceIDBuffer, material, instanceCount, primitive=cache.Primitive, cameraBuffer=camera->GetBuffer()](RHICommandBuffer* cmd) {
					cmd->BindGraphicsPipeline(pso);
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
					cmd->SetShaderParam(1, 0, RHIShaderParam::StructuredBuffer(instanceBuffer));
					cmd->SetShaderParam(1, 1, RHIShaderParam::StructuredBuffer(instanceIDBuffer));
					material->BindBasePassShaderPrams(cmd, true);
					cmd->BindVertexBuffer(primitive->VertexBuffer, 0, 0);
					cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
					cmd->DrawIndexed(primitive->IndexCount, instanceCount, 0, 0, 0);
				});
			}
		}
	}

	void PrimitiveInstancedRendererCPUDriven::GenerateDirectionalShadowDrawCall(Object::ShadowCamera* camera, RHIGraphicsPipelineState* pso) {
		Render::DrawCallQueue& queue = camera->GetRenderContext().RenderingQueue;
		const Math::Frustum& frustum = camera->GetFrustum();
		const auto& renderingIndices = m_RenderingCacheArray[EnumCast(ERenderPassType::DirectionalShadow)];
		for(uint32 i : renderingIndices) {
			InstancedPrimitiveCacheGroup& group = m_PrimitiveStorage.Get(i);
			TArray<uint32> instanceIDs;
			group.DataMgr->GenerateInstanceID(frustum, instanceIDs);
			if(instanceIDs.IsEmpty()) {
				continue;
			}
			RHIDynamicBuffer instanceIDBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::SRV,
				instanceIDs.ByteSize(), instanceIDs.Data(), sizeof(uint32));
			RHIBuffer* instanceBuffer = group.DataMgr->GetInstanceBuffer();
			uint32 instanceCount = instanceIDs.Size();
			for(auto& cache: group.PrimitiveCaches) {
				queue.PushDrawCall([pso, instanceBuffer, instanceIDBuffer, instanceCount, primitive=cache.Primitive, cameraBuffer=camera->GetBuffer()](RHICommandBuffer* cmd) {
					cmd->BindGraphicsPipeline(pso);
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
					cmd->SetShaderParam(1, 0, RHIShaderParam::StructuredBuffer(instanceBuffer));
					cmd->SetShaderParam(1, 1, RHIShaderParam::StructuredBuffer(instanceIDBuffer));
					cmd->BindVertexBuffer(primitive->VertexBuffer, 0, 0);
					cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
					cmd->DrawIndexed(primitive->IndexCount, instanceCount, 0, 0, 0);
				});
			}
		}
	}

#pragma region GPUDriven

	static constexpr uint32 NUM_THREAD_PER_GROUP = 256;

	struct CullingParams {
		Math::Frustum Frustum;
		Math::FMatrix4x4 ViewProjectMatrix;
		Math::FVector4 HZBSizeScale; // (width scale, height scale, unused, unused)
	};

	struct GPUAABBData {
		Math::FVector4 AABBMin; // (AABBMin.xyz, instance start)
		Math::FVector4 AABBMax; // (AABBMax.xyz, instanec end);
	};

	struct ClusterData {
		uint32 ClusterIndex;
		uint32 SrcInstanceOffset;
		uint32 DstInstanceOffset;
		uint32 PrimitiveStart;
		uint32 PrimitiveCount;
	};

	struct InstanceDrawData {
		DrawIndexedIndirectCommand Command;
	};

	PrimitiveInstancedRendererGPUDriven::PrimitiveInstancedRendererGPUDriven(PrimitiveMaterialPSOCache* cache):
	PrimitiveInstancedRendererBase(cache),
	m_IsBufferDirty(false),
	m_EnableOcclusionTest(true){
		{
			RHIComputePipelineStateDesc desc;
			InstanceCullingCS::ShaderPermutation cp;
			cp.OCCLUSION_TEST = true;
			desc.Shader = Render::GlobalShaderMap::Instance()->GetShader<InstanceCullingCS>(cp)->GetRHI();
			desc.Layout = {
				{
					{EBindingType::UniformBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::StructuredBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::StructuredBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::StructuredBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::RWStructuredBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::RWStructuredBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::Texture, EShaderStageFlags::Compute, 1},
					{EBindingType::Sampler, EShaderStageFlags::Compute, 1},
				}
			};
			m_CullingPSOWithOcclusionTest = RHI::Instance()->CreateComputePipelineState(desc);
		}
		// without occlusion test
		{
			RHIComputePipelineStateDesc desc;
			InstanceCullingCS::ShaderPermutation cp;
			cp.OCCLUSION_TEST = false;
			desc.Shader = Render::GlobalShaderMap::Instance()->GetShader<InstanceCullingCS>(cp)->GetRHI();
			desc.Layout = {
				{
					{EBindingType::UniformBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::StructuredBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::StructuredBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::StructuredBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::RWStructuredBuffer, EShaderStageFlags::Compute, 1},
					{EBindingType::RWStructuredBuffer, EShaderStageFlags::Compute, 1},
				}
			};
			m_CullingPSO = RHI::Instance()->CreateComputePipelineState(desc);
		}
		m_SRVAlignment = RHI::Instance()->GetBufferAlignment(EBufferFlags::SRV);
	}

	void PrimitiveInstancedRendererGPUDriven::Add(MeshECSComponent* mesh, InstancedDataECSComponent* instanceData) {
		uint32& indexRef = instanceData->CacheIndex;
		if (indexRef == INVALID_INDEX) {
			indexRef = m_PrimitiveStorage.Allocate();
			InstancedPrimitiveCacheGroup& cacheGroup = m_PrimitiveStorage.Get(indexRef);
			cacheGroup.DataMgr = &instanceData->InstanceData;
			cacheGroup.PrimitiveCaches.Reserve(mesh->Primitives.Size());
			for (PrimitiveRenderData& primitiveData : mesh->Primitives) {
				InstancedPrimitiveCache& cache = cacheGroup.PrimitiveCaches.EmplaceBack();
				cache.Material = primitiveData.Material;
				cache.MaterialIndex = INVALID_INDEX;
				cache.Primitive = primitiveData.Primitive;
				m_MaterialPSOCache->AddMaterialRef(&cache);
			}
			m_IsBufferDirty = true;
		}
		InstancedPrimitiveCacheGroup& cacheGroup = m_PrimitiveStorage.Get(indexRef);
		++cacheGroup.RefCount;
		if(cacheGroup.RenderFlag != mesh->RenderFlags) {
			m_IsBufferDirty = true;
			cacheGroup.RenderFlag = mesh->RenderFlags;
		}
		++m_PrimitiveStorage.Get(indexRef).RefCount;
	}

	void PrimitiveInstancedRendererGPUDriven::Reset() {
		for(uint32 i=0; i<m_PrimitiveStorage.Size(); ++i) {
			InstancedPrimitiveCacheGroup& group = m_PrimitiveStorage.Get(i);
			if(group.IsValid()) {
				if(0 == group.RefCount) {
					for (InstancedPrimitiveCache& cache : group.PrimitiveCaches) {
						m_MaterialPSOCache->RemoveMaterialRef(&cache);
					}
					m_PrimitiveStorage.Free(i);
					m_IsBufferDirty = true;
				}
				group.RefCount = 0;
			}
		}
	}

	void PrimitiveInstancedRendererGPUDriven::PreDrawCall() {
		if(m_IsBufferDirty || CheckBufferRetired()) {
			UpdateGPUBuffers();
			m_IsBufferDirty = false;
		}
	}

	void PrimitiveInstancedRendererGPUDriven::GenerateBasePassDrawCall(Object::RenderCamera* camera) {
		if(!m_PassBuffersArray[EnumCast(ERenderPassType::BasePass)].ClusterSize) {
			return;
		}
		RenderContext& context = camera->GetRenderContext();
		GPUCulling(camera, context, ERenderPassType::BasePass);

		const RHIDynamicBuffer cameraBuffer = camera->GetBuffer();
		RHIBuffer* instanceIDBuffer = context.CullResultBuffer;
		RHIBuffer* indirectCmdBuffer = context.IndirectCmdBuffer;

		uint32 primitiveIndex = 0;
		uint32 alignedInstanceOffset = 0;
		for(uint32 groupIndex: m_RenderingCacheArray[EnumCast(ERenderPassType::BasePass)]) {
			InstancedPrimitiveCacheGroup& group = m_PrimitiveStorage.Get(groupIndex);
			CHECK(group.IsValid());
			RHIBuffer* instanceBuffer = group.DataMgr->GetInstanceBuffer();
			const uint32 alignedInstanceSize = AlignInstanceSize(group.DataMgr->GetInstances().Size());
			for(InstancedPrimitiveCache& cache: group.PrimitiveCaches) {
				MaterialInterface* material = cache.Material;
				RHIGraphicsPipelineState* pso = m_MaterialPSOCache->GetPSO(&cache, true);
				context.RenderingQueue.PushDrawCall([cameraBuffer, indirectCmdBuffer, primitiveIndex, instanceIDBuffer, instanceBuffer, pso, material, alignedInstanceOffset, alignedInstanceSize, primitive=cache.Primitive](RHICommandBuffer* cmd) {
					cmd->BindGraphicsPipeline(pso);
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
					cmd->SetShaderParam(1, 0, RHIShaderParam::StructuredBuffer(instanceBuffer));
					cmd->SetShaderParam(1, 1, RHIShaderParam::StructuredBuffer(instanceIDBuffer,
						alignedInstanceOffset * sizeof(uint32), alignedInstanceSize * sizeof(uint32)));
					material->BindBasePassShaderPrams(cmd, true);
					cmd->BindVertexBuffer(primitive->VertexBuffer, 0, 0);
					cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
					cmd->DrawIndexedIndirect(indirectCmdBuffer, primitiveIndex * sizeof(InstanceDrawData), 1);
				});
				++primitiveIndex;
			}
			alignedInstanceOffset += alignedInstanceSize;
		}
	}

	void PrimitiveInstancedRendererGPUDriven::GenerateDirectionalShadowDrawCall(Object::ShadowCamera* camera, RHIGraphicsPipelineState* pso) {
		if (!m_PassBuffersArray[EnumCast(ERenderPassType::DirectionalShadow)].ClusterSize) {
			return;
		}
		RenderContext& context = camera->GetRenderContext();
		GPUCulling(camera, context, ERenderPassType::DirectionalShadow);

		const RHIDynamicBuffer cameraBuffer = camera->GetBuffer();
		RHIBuffer* instanceIDBuffer = context.CullResultBuffer;
		RHIBuffer* indirectCmdBuffer = context.IndirectCmdBuffer;

		uint32 primitiveIndex = 0;
		uint32 alignedInstanceOffset = 0;
		for (uint32 groupIndex : m_RenderingCacheArray[EnumCast(ERenderPassType::DirectionalShadow)]) {
			InstancedPrimitiveCacheGroup& group = m_PrimitiveStorage.Get(groupIndex);
			CHECK(group.IsValid());
			RHIBuffer* instanceBuffer = group.DataMgr->GetInstanceBuffer();
			const uint32 alignedInstanceSize = AlignInstanceSize(group.DataMgr->GetInstances().Size());
			for (InstancedPrimitiveCache& cache : group.PrimitiveCaches) {
				context.RenderingQueue.PushDrawCall([cameraBuffer, indirectCmdBuffer, primitiveIndex, instanceIDBuffer, instanceBuffer, pso, alignedInstanceOffset, alignedInstanceSize, primitive=cache.Primitive](RHICommandBuffer* cmd) {
					cmd->BindGraphicsPipeline(pso);
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraBuffer));
					cmd->SetShaderParam(1, 0, RHIShaderParam::StructuredBuffer(instanceBuffer));
					cmd->SetShaderParam(1, 1, RHIShaderParam::StructuredBuffer(instanceIDBuffer,
						alignedInstanceOffset * sizeof(uint32), alignedInstanceSize * sizeof(uint32)));
					cmd->BindVertexBuffer(primitive->VertexBuffer, 0, 0);
					cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
					cmd->DrawIndexedIndirect(indirectCmdBuffer, primitiveIndex * sizeof(InstanceDrawData), 1);
				});
				++primitiveIndex;
			}
			alignedInstanceOffset += alignedInstanceSize;
		}
	}

	bool PrimitiveInstancedRendererGPUDriven::CheckBufferRetired() {
		for(uint32 i=0; i<m_PrimitiveStorage.Size(); ++i) {
			const InstancedPrimitiveCacheGroup& group = m_PrimitiveStorage.Get(i);
			if(group.IsValid()) {
				// data removed
				if (!group.RefCount) {
					return true;
				}
				// buffer updated
				if (group.DataMgr->GetInstanceBuffer() != group.CacheInstanceBuffer) {
					return true;
				}
			}
		}
		return false;
	}

	void PrimitiveInstancedRendererGPUDriven::UpdateGPUBuffers() {
		// combine all instance buffer
		constexpr auto passCount = EnumCast(ERenderPassType::MaxNum);

		TArray<GPUAABBData> instanceAABBs;
		TArray<GPUAABBData> clusterAABBs;
		TStaticArray<TArray<ClusterData>, passCount> clusterDatasArray;
		TStaticArray<TArray<InstanceDrawData>, passCount> indirectCmdsArray;
		TStaticArray<uint32, passCount> alignedInstanceCountArray(0);
		TStaticArray<uint32, passCount> primitiveCountArray(0);
		TStaticArray<uint32, passCount> groupCountArray(0);
		TStaticArray<uint32, passCount> clusterCountArray(0);

		for(auto& indices: m_RenderingCacheArray) {
			indices.Reset();
			indices.Reserve(m_PrimitiveStorage.Size());
		}

		for(uint32 groupIndex=0; groupIndex<m_PrimitiveStorage.Size();++groupIndex) {
			InstancedPrimitiveCacheGroup& group = m_PrimitiveStorage.Get(groupIndex);
			if(!group.IsValid()) {
				continue;
			}
			if(group.DataMgr->IsEmpty()) {
				group.CacheInstanceBuffer = nullptr;
				continue;
			}
			group.CacheInstanceBuffer = group.DataMgr->GetInstanceBuffer();

			// collect global data
			const uint32 clusterOffset = clusterAABBs.Size();
			const uint32 instanceOffset = instanceAABBs.Size();
			for (auto& aabb : group.DataMgr->GetInstanceAABBs()) {
				GPUAABBData& aabbData = instanceAABBs.EmplaceBack();
				aabbData.AABBMin = Math::FVector4(aabb.Min, 1.0f);
				aabbData.AABBMax = Math::FVector4(aabb.Max, 1.0f);
			}
			for (auto& cluster : group.DataMgr->GetClusters()) {
				GPUAABBData& clusterData = clusterAABBs.EmplaceBack();
				clusterData.AABBMin = Math::FVector4(cluster.AABB.Min, (float)cluster.InstanceStart);
				clusterData.AABBMax = Math::FVector4(cluster.AABB.Max, (float)cluster.InstanceEnd);
			}
			// collect per pass data
			const uint32 primitiveCount = group.PrimitiveCaches.Size();
			const uint32 instanceCount = group.DataMgr->GetInstances().Size();
			// offset must be aligned by srv buffer requirement
			const uint32 alignedInstanceCount = Math::AlignUp(instanceCount, m_SRVAlignment);
			const uint32 clusterCount = group.DataMgr->GetClusters().Size();
			for(uint32 pass=0; pass< passCount; ++pass) {
				if(group.RenderFlag.HasFlag((ERenderPassType)pass)) {
					m_RenderingCacheArray[pass].PushBack(groupIndex);
					for (auto& cache : group.PrimitiveCaches) {
						auto& cmd = indirectCmdsArray[pass].EmplaceBack().Command;
						cmd.IndexCount = cache.Primitive->IndexCount;
						cmd.InstanceCount = 0;
						cmd.FirstIndex = 0;
						cmd.FirstVertex = 0;
						cmd.FirstInstance = 0;
					}
					for(uint32 i=0; i<clusterCount; ++i) {
						auto& clusterData = clusterDatasArray[pass].EmplaceBack();
						clusterData.ClusterIndex = clusterOffset + i;
						clusterData.SrcInstanceOffset = instanceOffset;
						clusterData.DstInstanceOffset = alignedInstanceCountArray[pass];
						clusterData.PrimitiveStart = primitiveCountArray[pass];
						clusterData.PrimitiveCount = primitiveCount;
					}
					clusterCountArray[pass] += clusterCount;
					alignedInstanceCountArray[pass] += alignedInstanceCount;
					primitiveCountArray[pass] += primitiveCount;
					++groupCountArray[pass];
				}
			}
		}

		// build global buffers
		{
			// instance aabbs
			if (instanceAABBs.Size()) {
				if (!m_InstanceAABBBuffer || m_InstanceAABBBuffer->GetDesc().ByteSize < instanceAABBs.ByteSize()) {
					m_InstanceAABBBuffer = RHI::Instance()->CreateBuffer({ EBufferFlags::SRV | EBufferFlags::CopyDst,
						instanceAABBs.ByteSize(), sizeof(GPUAABBData) });
					m_InstanceAABBBuffer->SetName("InstanceAABB");
				}
				m_InstanceAABBBuffer->UpdateData(instanceAABBs.Data(), instanceAABBs.ByteSize(), 0);
			}
			else {
				m_InstanceAABBBuffer.Reset();
			}

			// clusters
			if (clusterAABBs.Size()) {
				if (!m_ClusterAABBBuffer || m_ClusterAABBBuffer->GetDesc().ByteSize < clusterAABBs.ByteSize()) {
					m_ClusterAABBBuffer = RHI::Instance()->CreateBuffer({ EBufferFlags::SRV | EBufferFlags::CopyDst,
						clusterAABBs.ByteSize(), sizeof(GPUAABBData) });
					m_ClusterAABBBuffer->SetName("ClusterAABB");
				}
				m_ClusterAABBBuffer->UpdateData(clusterAABBs.Data(), clusterAABBs.ByteSize(), 0);
			}
			else {
				m_ClusterAABBBuffer.Reset();
			}
		}

		// build per pass buffers
		for(uint32 pass=0; pass<passCount; ++pass) {
			PassBuffers& buffers = m_PassBuffersArray[pass];
			// cluster data
			auto& clusterDatas = clusterDatasArray[pass];
			if(clusterDatas.Size()) {
				if(!buffers.Cluster || buffers.Cluster->GetDesc().ByteSize < clusterDatas.ByteSize()) {
					buffers.Cluster = RHI::Instance()->CreateBuffer({ EBufferFlags::SRV | EBufferFlags::CopyDst,
						clusterDatas.ByteSize(), sizeof(ClusterData) });
					buffers.Cluster->SetName("Cluster");
				}
				buffers.Cluster->UpdateData(clusterDatas.Data(), clusterDatas.ByteSize(), 0);
			}
			else {
				buffers.Cluster.Reset();
			}

			// indirect draw cmd
			auto& indirectCmds = indirectCmdsArray[pass];
			if(indirectCmds.Size()) {
				if (!buffers.IndirectCmd || buffers.IndirectCmd->GetDesc().ByteSize < indirectCmds.ByteSize()) {
					buffers.IndirectCmd = RHI::Instance()->CreateBuffer({ EBufferFlags::CopySrc,
						indirectCmds.ByteSize(), sizeof(InstanceDrawData) });
					buffers.IndirectCmd->SetName("IndirectCmd");
				}
				buffers.IndirectCmd->UpdateData(indirectCmds.Data(), indirectCmds.ByteSize(), 0);
			}

			buffers.ClusterSize = clusterCountArray[pass];
			buffers.AlignedInstanceSize = alignedInstanceCountArray[pass];
			buffers.PrimitiveSize = primitiveCountArray[pass];
		}
	}

	void PrimitiveInstancedRendererGPUDriven::GPUCulling(Object::RenderCameraBase* camera, RenderContext& context, ERenderPassType pass) {
		PassBuffers& buffer = m_PassBuffersArray[EnumCast(pass)];
		RHITexture* hzb = context.HZB ? context.HZB->GetTexture() : nullptr;

		//  check instance id buffer
		const uint32 instanceIDByteSize = buffer.AlignedInstanceSize * sizeof(uint32);
		if(!context.CullResultBuffer || context.CullResultBuffer->GetDesc().ByteSize < instanceIDByteSize) {
			context.CullResultBuffer = RHI::Instance()->CreateBuffer({ EBufferFlags::SRV | EBufferFlags::UAV,
				instanceIDByteSize, sizeof(uint32) });
			context.CullResultBuffer->SetName("CullingResult");
		}

		// check indirect cmd buffer
		const uint32 indirectCmdByteSize = buffer.PrimitiveSize * sizeof(InstanceDrawData);
		if(!context.IndirectCmdBuffer || context.IndirectCmdBuffer->GetDesc().ByteSize < indirectCmdByteSize) {
			context.IndirectCmdBuffer = RHI::Instance()->CreateBuffer({ EBufferFlags::UAV | EBufferFlags::CopyDst | EBufferFlags::IndirectDraw,
				indirectCmdByteSize, sizeof(InstanceDrawData) });
			context.IndirectCmdBuffer->SetName("IndirectCmd");
		}

		// culling param data
		CullingParams params;
		params.Frustum = camera->GetFrustum();
		if (hzb) {
			params.ViewProjectMatrix = context.HZB->GetViewProjectMatrix();
			auto actualSize = context.HZB->GetActualSize();
			params.HZBSizeScale.X = (float)actualSize.w / (float)hzb->GetDesc().Width;
			params.HZBSizeScale.Y = (float)actualSize.h / (float)hzb->GetDesc().Height;
		}
		RHIDynamicBuffer paramBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(params), &params, 0);

		uint32 numThreadGroup = (buffer.ClusterSize + NUM_THREAD_PER_GROUP - 1) / NUM_THREAD_PER_GROUP;
		RHIBuffer* srcIndirectCmdBuffer = buffer.IndirectCmd.Get();
		RHIBuffer* indirectCmdBuffer = context.IndirectCmdBuffer.Get();
		RHIBuffer* cullResultBuffer = context.CullResultBuffer.Get();
		RHIBuffer* clusterDataBuffer = buffer.Cluster.Get();
		context.CullingQueue.PushDrawCall([this, paramBuffer, hzb, numThreadGroup, clusterDataBuffer, indirectCmdBuffer, cullResultBuffer, srcIndirectCmdBuffer](RHICommandBuffer* cmd) {
			const bool bEnableOcclusionTest = hzb && m_EnableOcclusionTest;
			cmd->TransitionBufferState(indirectCmdBuffer, EResourceState::Unknown, EResourceState::TransferDst);
			cmd->CopyBufferToBuffer(srcIndirectCmdBuffer, indirectCmdBuffer, 0, 0, indirectCmdBuffer->GetDesc().ByteSize);
			cmd->TransitionBufferState(indirectCmdBuffer, EResourceState::TransferDst, EResourceState::UnorderedAccessView);
			RHIComputePipelineState* pso = bEnableOcclusionTest ? m_CullingPSOWithOcclusionTest.Get() : m_CullingPSO.Get();
			cmd->BindComputePipeline(pso);
			cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(paramBuffer));
			cmd->SetShaderParam(0, 1, RHIShaderParam::StructuredBuffer(m_InstanceAABBBuffer.Get()));
			cmd->SetShaderParam(0, 2, RHIShaderParam::StructuredBuffer(m_ClusterAABBBuffer.Get()));
			cmd->SetShaderParam(0, 3, RHIShaderParam::StructuredBuffer(clusterDataBuffer));
			cmd->SetShaderParam(0, 4, RHIShaderParam::RWStructuredBuffer(indirectCmdBuffer));
			cmd->SetShaderParam(0, 5, RHIShaderParam::RWStructuredBuffer(cullResultBuffer));
			if(bEnableOcclusionTest) {
				cmd->SetShaderParam(0, 6, RHIShaderParam::Texture(hzb));
				RHISampler* sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Point, ESamplerAddressMode::Clamp);
				cmd->SetShaderParam(0, 7, RHIShaderParam::Sampler(sampler));
			}
			cmd->Dispatch(numThreadGroup, 1, 1);
			cmd->TransitionBufferState(indirectCmdBuffer, EResourceState::UnorderedAccessView, EResourceState::IndirectDrawBuffer);
		});
	}

	uint32 PrimitiveInstancedRendererGPUDriven::AlignInstanceSize(uint32 size) const {
		return Math::AlignUp(size, m_SRVAlignment);
	}
#pragma endregion
}
