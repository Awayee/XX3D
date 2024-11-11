#pragma once
#include "Objects/Public/ECS.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/InstanceDataMgr.h"
#include "Objects/Public/Material.h"
#include "Objects/Public/RenderResource.h"
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/Container.h"


namespace Object {
	class PrimitiveRendererBase;
	class PrimitiveInstancedRendererBase;

	enum class ERenderPassType : uint32 {
		PrePass = 0,
		BasePass,
		DirectionalShadow,
		MaxNum
	};

	struct ObjectTransformData {
		Math::FMatrix4x4 ObjectMatrix;
		Math::FMatrix4x4 ObjectInvMatrix;
		void SetTransform(const Math::FTransform& transform);
	};

	struct PrimitiveRenderData {
		PrimitiveResource* Primitive;
		MaterialInterface* Material;
		uint32 MaterialIndex{ INVALID_INDEX };
	};

	// static mesh
	struct MeshECSComponent {
		TArray<PrimitiveRenderData> Primitives;
		Math::AABB3 AABB;
		TEnumFlag<ERenderPassType> RenderFlags;
		REGISTER_ECS_COMPONENT(MeshECSComponent);
	};

	// transform
	struct TransformECSComponent {
		ObjectTransformData TransformData;
		Math::FTransform Transform;
		void SetTransform(const Math::FTransform& transform);
		REGISTER_ECS_COMPONENT(TransformECSComponent);
	};

	// instanced static mesh
	struct InstancedDataECSComponent {
		InstanceDataMgr InstanceData;
		REGISTER_ECS_COMPONENT(InstancedDataECSComponent);
	};

	class MeshRenderSystem : public ECSSystem<TransformECSComponent, MeshECSComponent> {
	private:
		void Update(ECSScene* ecsScene, TransformECSComponent* transform, MeshECSComponent* meshCom) override;
		RENDER_SCENE_REGISTER_SYSTEM(MeshRenderSystem);
	};

	class InstancedMeshRenderSystem : public ECSSystem<MeshECSComponent, InstancedDataECSComponent> {
	private:
		void Update(ECSScene* ecsScene, MeshECSComponent* meshCom, InstancedDataECSComponent* instanceCom) override;
		RENDER_SCENE_REGISTER_SYSTEM(InstancedMeshRenderSystem);
	};

	// Manage all materials and pipeline states with reference counter, the pso with no reference will be removed.
	class PrimitiveMaterialPSOCache {
	public:
		uint32 FindOrAddMaterial(MaterialInterface* material);
		MaterialInterface* GetMaterial(uint32 materialIndex) const;
		RHIGraphicsPipelineState* GetPSO(uint32 materialIndex, bool bInstanced);
		uint32 GetMaterialSize() const;
		// Reset all data
		void Reset();
		// clean materials and pipelines with no reference, and reset counter. called after rendering.
		void Clean();

	private:
		struct PSOCache {
			RHIGraphicsPipelineStatePtr PSO;
			uint32 MaterialHash;
			uint32 RefCounter;
		};

		struct MaterialCache {
			MaterialInterface* Material;
			uint32 RefCounter;
			TStaticArray<uint32, 2> PSOIndex;
		};

		TIDMapContainer<PSOCache> m_PSOCaches;
		TArray<MaterialCache> m_MaterialCaches;
		TMap<MaterialInterface*, uint32> m_MapMaterialIndex;
	};

	class PrimitiveMgr {
	public:
		PrimitiveMgr();
		~PrimitiveMgr() = default;
		void AddPrimitive(const ObjectTransformData& transform, TArrayView<PrimitiveRenderData> primitives, TEnumFlag<ERenderPassType> passFlag);
		void AddInstancedPrimitive(InstanceDataMgr* instanceDataMgr, TArrayView<PrimitiveRenderData> primitives, TEnumFlag<ERenderPassType> passFlag);

		void GenerateBasePassDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer,
			Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue);

		void GenerateDirectionalShadowDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer,
			Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue,
			RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO);

		void Clean();
	protected:
		TUniquePtr<PrimitiveMaterialPSOCache> m_MaterialPSOCache;
		TUniquePtr<PrimitiveRendererBase> m_PrimitiveRenderer;
		TUniquePtr<PrimitiveInstancedRendererBase> m_PrimitiveInstancedRenderer;
		// get the corrected index of material in primitive
		void CorrectPrimitiveMaterialIndex(PrimitiveRenderData* primitive);
	};

	class IPrimitiveRendererBase {
	public:
		IPrimitiveRendererBase(PrimitiveMaterialPSOCache* materialPSOCache) : m_MaterialPSOCache(materialPSOCache) {}
		virtual ~IPrimitiveRendererBase() = default;
		virtual void GenerateBasePassDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer,
			Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue) = 0;

		virtual void GenerateDirectionalShadowDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer,
			Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue,
			RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO) = 0;

	protected:
		PrimitiveMaterialPSOCache* m_MaterialPSOCache;
	};

	// primitive renderer for separated objects
	class PrimitiveRendererBase : public IPrimitiveRendererBase {
	public:
		using IPrimitiveRendererBase::IPrimitiveRendererBase;
		virtual void Add(const ObjectTransformData& transform, TArrayView<PrimitiveRenderData> primitives, TEnumFlag<ERenderPassType> passFlag) = 0;
		virtual void Reset() = 0;
	};

	// primitive renderer for instanced objects
	class PrimitiveInstancedRendererBase : public IPrimitiveRendererBase {
	public:
		using IPrimitiveRendererBase::IPrimitiveRendererBase;
		virtual void Add(InstanceDataMgr* instanceDataMgr, TArrayView<PrimitiveRenderData> primitives, TEnumFlag<ERenderPassType> passFlag) = 0;
		virtual void Reset() = 0;
	};

	class PrimitiveRendererCPUDriven : public PrimitiveRendererBase {
	public:
		using PrimitiveRendererBase::PrimitiveRendererBase;
		~PrimitiveRendererCPUDriven() override = default;
		void Add(const ObjectTransformData& transform, TArrayView<PrimitiveRenderData> primitives, TEnumFlag<ERenderPassType> passFlag) override;
		void Reset() override;
		void GenerateBasePassDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue) override;
		void GenerateDirectionalShadowDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue, RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO) override;
	private:
		// Multi primitives can share with one transform 
		TArray<ObjectTransformData> m_Transforms;

		// aabbs of primitives
		TArray<Math::AABB3> m_PrimitiveAABBs;
		// unbatched primitives
		struct PrimitiveCache {
			PrimitiveResource* Primitive;
			uint32 MaterialIndex;
			uint32 TransformIndex;
			uint32 AABBIndex;
		};
		TStaticArray<TArray<PrimitiveCache>, EnumCast(ERenderPassType::MaxNum)> m_PrimitiveCacheArrays;
		// frustum cull, and generate transform buffer for primitives
		struct PrimitiveCullResult {
			TArray<uint32> Primitives;
			TArray<RHIDynamicBuffer> TransformBuffers;
		};
		void FrustumCull(const Math::Frustum& frustum, PrimitiveCullResult& outResult, ERenderPassType pass);
	};

	class PrimitiveInstancedRendererCPUDriven : public PrimitiveInstancedRendererBase {
	public:
		using PrimitiveInstancedRendererBase::PrimitiveInstancedRendererBase;
		~PrimitiveInstancedRendererCPUDriven() override = default;
		void Add(InstanceDataMgr* instanceDataMgr, TArrayView<PrimitiveRenderData> primitives, TEnumFlag<ERenderPassType> passFlag) override;
		void Reset() override;
		void GenerateBasePassDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue) override;
		void GenerateDirectionalShadowDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue, RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO) override;
	private:
		TArray<InstanceDataMgr*> m_InstanceDataMgrs;
		// instanced primitives
		struct InstancedPrimitiveCache {
			PrimitiveResource* Primitive;
			uint32 MaterialIndex;
			uint32 InstanceDataIndex;
		};
		TStaticArray<TArray<InstancedPrimitiveCache>, EnumCast(ERenderPassType::MaxNum)> m_InstancedPrimitiveCacheArrays;

		struct PrimitiveCullResult {
			TArray<TPair<uint32, uint32>> InstancedPrimitives; // the indices and counts of primitives
			TArray<RHIDynamicBuffer> InstanceBuffers;
		};
		void FrustumCull(const Math::Frustum& frustum, PrimitiveCullResult& outResult, ERenderPassType pass);
	};
}