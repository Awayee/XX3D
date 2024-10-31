#pragma once
#include "Objects/Public/ECS.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/InstanceDataMgr.h"
#include "Objects/Public/Material.h"
#include "Objects/Public/RenderResource.h"
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/Container.h"


namespace Object {
	// MeshRenderer is responsible for data organization of mesh and transform, and generating draw calls.
	// It can be treated as a pseudo ECS framework with only 3 components.

	struct PrimitiveRenderData {
		PrimitiveResource* Primitive;
		MaterialInterface* Material;
		uint32 BasePassBatchIndex;
		uint32 BasePassInstancedBatchIndex;
	};

	struct TransformData {
		Math::FMatrix4x4 ObjectMatrix;
		Math::FMatrix4x4 ObjectInvMatrix;
		void SetTransform(const Math::FTransform& transform);
	};

	struct PrimitiveRenderData2 {
		PrimitiveResource* Primitive;
		uint32 MaterialIndex;
	};

	// static mesh
	struct MeshECSComponent {
		TArray<PrimitiveRenderData2> Primitives;
		Math::AABB3 AABB;
		bool CastShadow;
		REGISTER_ECS_COMPONENT(MeshECSComponent);
	};

	// transform
	struct TransformECSComponent {
		struct MatrixData {
			Math::FMatrix4x4 Transform{ Math::FMatrix4x4::IDENTITY };
			Math::FMatrix4x4 InverseTransform{ Math::FMatrix4x4::IDENTITY };
		};
		void SetTransform(const Math::FTransform& transform);
		const MatrixData& GetMatrixData() const { return m_MatrixData; }
	protected:
		MatrixData m_MatrixData;
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


	// A mesh renderer with primitive data organization in a pass, just for generating draw calls. TODO for test
	class MeshRenderInterface {
	public:
		virtual ~MeshRenderInterface() = default;
		virtual void AddPrimitiveDrawData(const PrimitiveRenderData2& primitive, const RHIDynamicBuffer& transformBuffer) = 0;
		virtual void AdInstancedPrimitiveDrawData(const PrimitiveRenderData2& primitive, const RHIDynamicBuffer& instanceBuffer, uint32 instanceCount) = 0;
		virtual void GenerateDrawCall(const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& queue) = 0;
		virtual void Reset() = 0;
	protected:
		struct PrimitiveDrawData {
			RHIDynamicBuffer TransformBuffer;
			PrimitiveResource* Primitive;
			uint32 MaterialIndex;
		};

		struct InstancedPrimitiveDrawData {
			RHIDynamicBuffer InstanceBuffer;
			PrimitiveResource* Primitive;
			uint32 MaterialIndex;
			uint32 InstanceCount;
		};
	};

	class MeshRenderer2: public MeshRenderInterface {
	public:
		MeshRenderer2(MaterialContainer* materialContainer): m_MaterialContainer(materialContainer){}
		~MeshRenderer2() override = default;
		// add batch draw primitive
		void AddPrimitiveDrawData(const PrimitiveRenderData2& primitive, const RHIDynamicBuffer& transformBuffer) override;
		void AdInstancedPrimitiveDrawData(const PrimitiveRenderData2& primitive, const RHIDynamicBuffer& instanceBuffer, uint32 instanceCount) override;
		// generate draw call
		void GenerateDrawCall(const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& basePassQueue) override;
		// clean
		void Reset() override;
	private:
		template<class T> struct BassPassPSOBatch {
			uint32 PSOIndex{ INVALID_INDEX };
			TArray<T> Primitives;
		};
		MaterialContainer* m_MaterialContainer;
		struct PSOData {
			RHIGraphicsPipelineStatePtr PSO;
			uint32 MaterialHash;
		};
		TIDMapContainer<PSOData> m_PSOs;
		TArray<BassPassPSOBatch<PrimitiveDrawData>> m_BasePassPSOBatches;
		TArray<BassPassPSOBatch<InstancedPrimitiveDrawData>> m_BasePassInstancedBatches;
		uint32 FindOrCreatePSO(MaterialInterface* material, bool bInstanced);
	};

	class DirectionalLightMehRenderer: public MeshRenderInterface {
	public:
		DirectionalLightMehRenderer(): m_StaticPSO(nullptr), m_StaticPSOInstanced(nullptr){}
		~DirectionalLightMehRenderer() override = default;
		void SetPSO(RHIGraphicsPipelineState* pso, RHIGraphicsPipelineState* instancedPSO);
		void AddPrimitiveDrawData(const PrimitiveRenderData2& primitive, const RHIDynamicBuffer& transformBuffer) override;
		void AdInstancedPrimitiveDrawData(const PrimitiveRenderData2& primitive, const RHIDynamicBuffer& instanceBuffer, uint32 instanceCount) override;
		void GenerateDrawCall(const RHIDynamicBuffer& cameraBuffer, Render::DrawCallQueue& queue) override;
		void Reset() override;
	private:
		TArray<PrimitiveDrawData> m_BatchPrimitives;
		TArray<InstancedPrimitiveDrawData> m_InstancedBatchPrimitives;
		RHIGraphicsPipelineState* m_StaticPSO;
		RHIGraphicsPipelineState* m_StaticPSOInstanced;
	};
}