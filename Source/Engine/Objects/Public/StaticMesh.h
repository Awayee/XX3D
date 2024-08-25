#pragma once
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/DrawCall.h"
#include "Objects/Public/ECS.h"

namespace Object {
	// transform
	class TransformComponent {
	public:
		struct TransformInfo {
			Math::FMatrix4x4 TransformMat{ Math::FMatrix4x4::IDENTITY };
			Math::FMatrix4x4 InvTransformMat{ Math::FMatrix4x4::IDENTITY };
		};
		void SetPosition(const Math::FVector3& pos);
		void SetRotation(const Math::FQuaternion& rot);
		void SetScale(const Math::FVector3& scale);
		const Math::FVector3& GetPosition() const { return m_Position; }
		const Math::FQuaternion& GetRotation() const { return m_Rotation; }
		const Math::FVector3& GetScale() const { return m_Scale; }
		const TransformInfo& GetTransformInfo() const { return m_TransformInfo; }
		const Math::FMatrix4x4& GetTransformMat()const { return m_TransformInfo.TransformMat; }
		const Math::FMatrix4x4& GetInvTransformMat() const { return m_TransformInfo.InvTransformMat; }
	protected:
		Math::FVector3 m_Position{ 0,0,0 };
		Math::FQuaternion m_Rotation{ 0,0,0,1 };
		Math::FVector3 m_Scale{ 1,1,1 };
		TransformInfo m_TransformInfo;
		void UpdateMat();
		REGISTER_ECS_COMPONENT(TransformComponent);
	};

	// static mesh
	struct PrimitiveRenderData {
		uint32 VertexCount;
		uint32 IndexCount;
		RHIBufferPtr VertexBuffer;
		RHIBufferPtr IndexBuffer;
		RHITexture* Texture;
		Math::AABB3 AABB;
	};
	struct StaticMeshComponent {
		void BuildFromAsset(const Asset::MeshAsset& meshAsset);
		TArray<PrimitiveRenderData> Primitives;
		RHIBufferPtr UniformBuffer;
		REGISTER_ECS_COMPONENT(StaticMeshComponent);
	};

	class MeshRenderSystem: public ECSSystem<TransformComponent, StaticMeshComponent> {
	public:
		void Update(ECSScene* ecsScene, TransformComponent* transform, StaticMeshComponent* staticMesh) override;
		static void Initialize();
	};
}
