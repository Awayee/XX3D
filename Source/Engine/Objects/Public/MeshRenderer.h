#pragma once
#include "Objects/Public/ECS.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/InstanceDataMgr.h"
#include "Objects/Public/Material.h"
#include "Objects/Public/RenderResource.h"
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/Container.h"


namespace Object {
	struct RenderContext;
	class RenderCameraBase;
	class RenderCamera;
	class ShadowCamera;
	class PrimitiveRendererBase;
	class PrimitiveInstancedRendererBase;

	// TODO Discard mesh and instance components in ECS, storage all mesh data in PrimitiveMgr.

	enum class ERenderPassType: uint32 {
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
		uint32 CacheIndex{ INVALID_INDEX };
		void SetTransform(const Math::FTransform& transform);
		REGISTER_ECS_COMPONENT(TransformECSComponent);
	};

	// instanced static mesh
	struct InstancedDataECSComponent {
		InstanceDataMgr InstanceData;
		uint32 CacheIndex{INVALID_INDEX};
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
		struct MaterialChild {
			MaterialInterface* Material;
			uint32 MaterialIndex;
		};
		RHIGraphicsPipelineState* GetPSO(MaterialChild* handle, bool bInstanced);
		void AddMaterialRef(MaterialChild* handle);
		void RemoveMaterialRef(MaterialChild* handle);
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

		uint32 FixMaterialIndex(const MaterialChild* handle);
		uint32 FindOrAddMaterial(MaterialInterface* material);
		MaterialInterface* GetMaterial(uint32 materialIndex) const;
		RHIGraphicsPipelineState* FindOrAddPSO(uint32 materialIndex, bool bInstanced);
	};

	class PrimitiveMgr {
	public:
		PrimitiveMgr();
		~PrimitiveMgr() = default;

		void AddPrimitive(MeshECSComponent* mesh, TransformECSComponent* transform);

		void AddInstancedPrimitive(MeshECSComponent* mesh, InstancedDataECSComponent* instanceData);

		void PreDrawCall();

		void GenerateBasePassDrawCall(Object::RenderCamera* camera);

		void GenerateDirectionalShadowDrawCall(Object::ShadowCamera* camera, RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO);

		void Clean();
	protected:
		TUniquePtr<PrimitiveMaterialPSOCache> m_MaterialPSOCache;
		TUniquePtr<PrimitiveRendererBase> m_PrimitiveRenderer;
		TUniquePtr<PrimitiveInstancedRendererBase> m_PrimitiveInstancedRenderer;
	};

	class IPrimitiveRendererBase {
	public:
		IPrimitiveRendererBase(PrimitiveMaterialPSOCache* materialPSOCache) : m_MaterialPSOCache(materialPSOCache) {}
		virtual ~IPrimitiveRendererBase() = default;
		virtual void PreDrawCall() = 0;
		virtual void GenerateBasePassDrawCall(Object::RenderCamera* camera) = 0;
		virtual void GenerateDirectionalShadowDrawCall(Object::ShadowCamera* camera, RHIGraphicsPipelineState* pso) = 0;

	protected:
		PrimitiveMaterialPSOCache* m_MaterialPSOCache;
	};

	// primitive renderer for separated objects
	class PrimitiveRendererBase: public IPrimitiveRendererBase {
	public:
		using IPrimitiveRendererBase::IPrimitiveRendererBase;
		virtual void Add(MeshECSComponent* mesh, TransformECSComponent* transform) = 0;
		virtual void Reset() = 0;
	};

	// primitive renderer for instanced objects
	class PrimitiveInstancedRendererBase: public IPrimitiveRendererBase {
	public:
		using IPrimitiveRendererBase::IPrimitiveRendererBase;
		virtual void Add(MeshECSComponent* mesh, InstancedDataECSComponent* instanceData) = 0;
		virtual void Reset() = 0;
	};

	class PrimitiveRendererCPUDriven: public PrimitiveRendererBase {
	public:
		using PrimitiveRendererBase::PrimitiveRendererBase;
		~PrimitiveRendererCPUDriven() override = default;
		void Add(MeshECSComponent* mesh, TransformECSComponent* transform) override;
		void Reset() override;
		void PreDrawCall() override {/*Do nothing*/ }
		void GenerateBasePassDrawCall(Object::RenderCamera* camera) override;
		void GenerateDirectionalShadowDrawCall(Object::ShadowCamera* camera, RHIGraphicsPipelineState* pso) override;
	private:
		struct PrimitiveCache: PrimitiveMaterialPSOCache::MaterialChild{
			PrimitiveResource* Primitive;
			Math::AABB3 AABB;
		};
		struct PrimitiveCacheGroup {
			ObjectTransformData Transform;
			TArray<PrimitiveCache> PrimitiveCaches;
			uint32 BufferIndex{ INVALID_INDEX }; // index of transform buffer
			uint32 RefCount{ 0 };
			bool IsValid() const { return PrimitiveCaches.Size(); }
		};
		struct PCGDestructor {
			void operator()(PrimitiveCacheGroup& p) const {
				p.PrimitiveCaches.Reset();
			}
		};
		// all primitives
		TIDReuseArray<PrimitiveCacheGroup, PCGDestructor> m_PrimitiveStorage;
		// primitives indices in per pass
		TStaticArray<TArray<uint32>, EnumCast(ERenderPassType::MaxNum)> m_RenderingCacheArray;
		// dynamic buffer of per primitive group.
		TArray<RHIDynamicBuffer> m_TransformBuffers;
		const RHIDynamicBuffer& GetTransformBuffer(uint32 groupIndex);
	};

	class PrimitiveInstancedRendererCPUDriven: public PrimitiveInstancedRendererBase {
	public:
		using PrimitiveInstancedRendererBase::PrimitiveInstancedRendererBase;
		~PrimitiveInstancedRendererCPUDriven() override = default;
		void Add(MeshECSComponent* mesh, InstancedDataECSComponent* instanceData) override;
		void Reset() override;
		void PreDrawCall() override {/*Do nothing*/ }
		void GenerateBasePassDrawCall(Object::RenderCamera* camera) override;
		void GenerateDirectionalShadowDrawCall(Object::ShadowCamera* camera, RHIGraphicsPipelineState* pso) override;
	private:
		struct InstancedPrimitiveCache : PrimitiveMaterialPSOCache::MaterialChild {
			PrimitiveResource* Primitive;
		};
		struct InstancedPrimitiveCacheGroup {
			InstanceDataMgr* DataMgr;
			TArray<InstancedPrimitiveCache> PrimitiveCaches;
			uint32 RefCount;
			bool IsValid()const { return PrimitiveCaches.Size(); }
		};
		struct IPCGDestructor {
			void operator()(InstancedPrimitiveCacheGroup& p) const {
				p.PrimitiveCaches.Reset();
			}
		};
		// all primitives
		TIDReuseArray<InstancedPrimitiveCacheGroup, IPCGDestructor> m_PrimitiveStorage;
		// primitives indices in per pass
		TStaticArray<TArray<uint32>, EnumCast(ERenderPassType::MaxNum)> m_RenderingCacheArray;
	};

	// GPU driven instanced
	class PrimitiveInstancedRendererGPUDriven: public PrimitiveInstancedRendererBase {
	public:
		PrimitiveInstancedRendererGPUDriven(PrimitiveMaterialPSOCache* cache);
		~PrimitiveInstancedRendererGPUDriven() override = default;
		void Add(MeshECSComponent* mesh, InstancedDataECSComponent* instanceData) override;
		void Reset() override;
		void PreDrawCall() override;
		void GenerateBasePassDrawCall(Object::RenderCamera* camera) override;
		void GenerateDirectionalShadowDrawCall(Object::ShadowCamera* camera, RHIGraphicsPipelineState* pso) override;
	private:
		struct InstancedPrimitiveCache: PrimitiveMaterialPSOCache::MaterialChild {
			PrimitiveResource* Primitive;
		};
		struct InstancedPrimitiveCacheGroup {
			InstanceDataMgr* DataMgr;
			RHIBuffer* CacheInstanceBuffer; // check if buffer retired
			TArray<InstancedPrimitiveCache> PrimitiveCaches;
			TEnumFlag<ERenderPassType> RenderFlag;
			uint32 RefCount;
			bool IsValid()const { return PrimitiveCaches.Size(); }
		};
		struct IPCGDestructor {
			void operator()(InstancedPrimitiveCacheGroup& p) const {
				p.PrimitiveCaches.Reset();
			}
		};
		// all primitives
		TIDReuseArray<InstancedPrimitiveCacheGroup, IPCGDestructor> m_PrimitiveStorage;
		// global GPU data
		RHIBufferPtr m_InstanceAABBBuffer;
		RHIBufferPtr m_ClusterAABBBuffer;
		// per pass GPU data
		struct PassBuffers {
			RHIBufferPtr IndirectCmd; // raw indirect cmd buffer, copied to per camera when culling
			RHIBufferPtr Cluster;
			uint32 ClusterSize;
			uint32 AlignedInstanceSize; // all instance size aligned by 4
			uint32 PrimitiveSize;
		};
		TStaticArray<PassBuffers, EnumCast(ERenderPassType::MaxNum)> m_PassBuffersArray;
		TStaticArray<TArray<uint32>, EnumCast(ERenderPassType::MaxNum)> m_RenderingCacheArray;
		// pso
		RHIComputePipelineStatePtr m_CullingPSOWithOcclusionTest;
		RHIComputePipelineStatePtr m_CullingPSO;
		uint32 m_SRVAlignment;
		bool m_IsBufferDirty;
		bool m_EnableOcclusionTest;

		bool CheckBufferRetired(); // check if need update buffer, called before UpdateGPUBuffers

		void UpdateGPUBuffers(); // update when instances modified

		void GPUCulling(Object::RenderCameraBase* camera, RenderContext& context, ERenderPassType pass);

		uint32 AlignInstanceSize(uint32 size) const;
	};
}