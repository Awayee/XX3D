#pragma once
#include "Objects/Public/ECS.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/InstanceDataMgr.h"
#include "Objects/Public/Material.h"
#include "Objects/Public/RenderResource.h"
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/Container.h"


namespace Object {

	enum class ERenderPassType {
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

	struct PrimitiveRenderData2 {
		PrimitiveResource* Primitive;
		MaterialInterface* Material;
		uint32 MaterialIndex{ INVALID_INDEX };
	};

	// static mesh
	struct MeshECSComponent {
		TArray<PrimitiveRenderData2> Primitives;
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
	class MaterialPSOCache {
	public:
		uint32 FindOrAddMaterial(MaterialInterface* material);
		MaterialInterface* GetMaterial(uint32 materialIndex) const;
		uint32 GetPSOIndex(uint32 materialIndex, bool bInstanced);
		RHIGraphicsPipelineState* GetPSO(uint32 materialIndex, bool bInstanced);
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

	// Collects all primitives in a render scene with all passes
	class PrimitiveRenderer {
	public:
		PrimitiveRenderer(MaterialPSOCache* materialPSOCache) : m_MaterialPSOCache(materialPSOCache) {}
		virtual ~PrimitiveRenderer() = default;
		virtual void AddPrimitive(const ObjectTransformData& transform, TArrayView<PrimitiveRenderData2> primitives, TEnumFlag<ERenderPassType> passFlag) = 0;
		virtual void AddInstancedPrimitive(InstanceDataMgr* instanceDataMgr, TArrayView<PrimitiveRenderData2> primitives, TEnumFlag<ERenderPassType> passFlag) = 0;

		virtual void GenerateBasePassDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer,
			Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue) = 0;

		virtual void GenerateDirectionalShadowDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer,
			Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue,
			RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO) = 0;

		virtual void Clean() = 0;
	protected:
		MaterialPSOCache* m_MaterialPSOCache;
	};

	class PrimitiveRendererCPUDriven: public PrimitiveRenderer {
	public:
		using PrimitiveRenderer::PrimitiveRenderer;
		~PrimitiveRendererCPUDriven() override = default;
		void AddPrimitive(const ObjectTransformData& transform, TArrayView<PrimitiveRenderData2> primitives, TEnumFlag<ERenderPassType> passFlag) override;
		void AddInstancedPrimitive(InstanceDataMgr* instanceDataMgr, TArrayView<PrimitiveRenderData2> primitives, TEnumFlag<ERenderPassType> passFlag) override;

		void GenerateBasePassDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer,
			Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue) override;

		void GenerateDirectionalShadowDrawCall(const Math::Frustum& frustum, const RHIDynamicBuffer& cameraBuffer,
			Render::DrawCallQueue& cullingQueue, Render::DrawCallQueue& renderingQueue,
			RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO) override;

		void Clean() override;
	private:
		// may multi primitives share one transform or one instanced data mgr.
		TArray<ObjectTransformData> m_Transforms;
		TArray<InstanceDataMgr*> m_InstanceDataMgrs;

		// unbatched primitives
		struct PrimitiveCache {
			PrimitiveResource* Primitive;
			uint32 MaterialIndex;
			uint32 TransformIndex;
		};
		TArray<PrimitiveCache> m_PrimitiveCaches;
		// aabbs of primitives
		TArray<Math::AABB3> m_PrimitiveAABBs;

		// instanced primitives
		struct InstancedPrimitiveCache {
			PrimitiveResource* Primitive;
			uint32 MaterialIndex;
			uint32 InstanceDataIndex;
		};
		TArray<InstancedPrimitiveCache> m_InstancedPrimitiveCaches;

		// get the corrected index of material in primitive
		void CorrectPrimitiveMaterialIndex(PrimitiveRenderData2* primitive);

		// frustum cull, and generate transform buffer for primitives
		struct PrimitiveCullResult {
			TArray<uint32> Primitives;
			TArray<RHIDynamicBuffer> TransformBuffers;
			TArray<TPair<uint32, uint32>> InstancedPrimitives; // the indices and counts of primitives
			TArray<RHIDynamicBuffer> InstanceBuffers;
		};
		void FrustumCull(const Math::Frustum& frustum, PrimitiveCullResult& outResult);
	};
}