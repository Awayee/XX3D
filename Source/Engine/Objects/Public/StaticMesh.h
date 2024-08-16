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
		void SetPosition(const Math::FVector3& pos);
		void SetRotation(const Math::FQuaternion& rot);
		void SetScale(const Math::FVector3& scale);
		const Math::FVector3& GetPosition() const { return m_Position; }
		const Math::FQuaternion& GetRotation() const { return m_Rotation; }
		const Math::FVector3& GetScale() const { return m_Scale; }
		const Math::FMatrix4x4& GetTransformMat()const { return m_TransformMat; }
		const Math::FMatrix4x4& GetInvTransformMat() const { return m_InvTransformMat; }
	protected:
		Math::FVector3 m_Position{ 0,0,0 };
		Math::FQuaternion m_Rotation{ 0,0,0,1 };
		Math::FVector3 m_Scale{ 1,1,1 };
		Math::FMatrix4x4 m_TransformMat{ Math::FMatrix4x4::IDENTITY };
		Math::FMatrix4x4 m_InvTransformMat{ Math::FMatrix4x4::IDENTITY };
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
	};
	class StaticMeshComponent {
	public:
		void BuildFromAsset(const Asset::MeshAsset& meshAsset);
		void CreateDrawCall(Render::DrawCallQueue& queue);
		const TArray<PrimitiveRenderData>& GetPrimitives()const { return m_Primitives; }
	private:
		TArray<PrimitiveRenderData> m_Primitives;
		REGISTER_ECS_COMPONENT(StaticMeshComponent);
	};

	class MeshRenderSystem: public ECSSystem<TransformComponent, StaticMeshComponent> {
	public:
		void Update(ECSScene* ecsScene, EntityID entityID, TransformComponent* transform, StaticMeshComponent* staticMesh) override;
	};
}
