#pragma once
#include "Math/Public/Transform.h"
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/DrawCall.h"
#include "Objects/Public/ECS.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Level.h"
#include "Objects/Public/InstanceDataMgr.h"
#include "Objects/Public/RenderResource.h"

namespace Object {
	// The level components registered by REGISTER_LEVEL_COMPONENT handles game and editor logic;
	// the ECS components registered by REGISTER_ECS_COMPONENT handles rendering.

	// transform
	struct TransformECSComponent {
		struct MatrixData {
			Math::FMatrix4x4 Transform{ Math::FMatrix4x4::IDENTITY };
			Math::FMatrix4x4 InverseTransform{ Math::FMatrix4x4::IDENTITY };
		};
		void SetTransform(const Math::FTransform& transform);
		const Math::FTransform& GetTransform()const { return m_Transform; }
		const MatrixData& GetMatrixData() const { return m_MatrixData; }
	protected:
		Math::FTransform m_Transform;
		MatrixData m_MatrixData;
		void UpdateMat();
		REGISTER_ECS_COMPONENT(TransformECSComponent);
	};

	class TransformComponent: public LevelComponent {
	public:
		Math::FTransform Transform;
		void OnLoad(const Json::Value& val) override;
		void OnAdd() override;
		void OnRemove() override;
		void TransformUpdated();
	private:
		REGISTER_LEVEL_COMPONENT(TransformComponent);
	};

	// static mesh
	struct PrimitiveRenderData {
		RHIBuffer* VertexBuffer;
		RHIBuffer* IndexBuffer;
		uint32 VertexCount;
		uint32 IndexCount;
		Math::AABB3 AABB;
		RHITexture* Texture;
		void SetResource(const Object::PrimitiveResource& res);
	};
	struct MeshECSComponent {
		TArray<PrimitiveRenderData> Primitives;
		Math::AABB3 AABB;
		bool CastShadow;
		void BuildFromAsset(const Asset::MeshAsset& meshAsset);
		REGISTER_ECS_COMPONENT(MeshECSComponent);
	};

	class MeshRenderSystem: public ECSSystem<TransformECSComponent, MeshECSComponent> {
	private:
		void Update(ECSScene* ecsScene, TransformECSComponent* transform, MeshECSComponent* staticMesh) override;
		RENDER_SCENE_REGISTER_SYSTEM(MeshRenderSystem);
	};

	class MeshComponent: public LevelComponent {
	public:
		void OnLoad(const Json::Value& val) override;
		void OnAdd() override;
		void OnRemove() override;
		void SetMeshFile(const XString& meshFile);
		const XString& GetMeshFile() const { return m_MeshFile; }
		void SetCastShadow(bool bCastShadow);
		bool GetCastShadow() const { return m_CastShadow; }
	private:
		XString m_MeshFile;
		bool m_CastShadow;
		REGISTER_LEVEL_COMPONENT(MeshComponent);
	};

	// instanced static mesh
	struct InstancedDataECSComponent{
		InstanceDataMgr InstanceDatas;
		void BuildInstances(const XString& instanceFile);
		void BuildInstances(const Math::AABB3& resAABB, TArray<Math::FTransform>&& instances);
		REGISTER_ECS_COMPONENT(InstancedDataECSComponent);
	};

	class InstancedMeshRenderSystem: public ECSSystem<MeshECSComponent, InstancedDataECSComponent> {
	public:
		static bool EnableGPUDriven;
	private:
		void Update(ECSScene* ecsScene, MeshECSComponent* meshCom, InstancedDataECSComponent* instanceCom) override;
		void UpdateWithGPUDriven(ECSScene* ecsScene, MeshECSComponent* meshCom, InstancedDataECSComponent* instanceCom);
		RENDER_SCENE_REGISTER_SYSTEM(InstancedMeshRenderSystem);
	};

	class InstanceDataComponent: public LevelComponent {
	public:
		void OnLoad(const Json::Value& val) override;
		void OnAdd() override;
		void OnRemove() override;
		void SetInstanceFile(const XString& file);
		const XString& GetInstanceFile() const { return m_InstanceFile; }
	private:
		XString m_InstanceFile;
		REGISTER_LEVEL_COMPONENT(InstanceDataComponent);
	};
}
