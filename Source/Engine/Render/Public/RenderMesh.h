#pragma once
#include "Core/Public/Math/Math.h"
#include "Core/Public/SmartPointer.h"
#include "Asset/Public/MeshAsset.h"
#include "RenderResources.h"
#include "RenderCommon.h"
#include "RenderScene.h"

namespace Engine {
	// static mesh
	class RenderMesh: public RenderObject {
	private:
		TVector<TUniquePtr<Primitive>> m_Primitives;
		TVector<Material*> m_Materials;
		Math::FMatrix4x4 m_TransformMat{ Math::FMatrix4x4::IDENTITY };
		Math::FMatrix4x4 m_InvTransformMat{ Math::FMatrix4x4::IDENTITY };
		BufferCommon m_TransformUniform;
		Engine::RDescriptorSet* m_TransformDescs{nullptr};
		//RHI::RDescriptorSet* m_DescriptorSet;
	public:
		RenderMesh(const AMeshAsset* meshAsset, RenderScene* scene);
		void DrawCall(Engine::RCommandBuffer* cmd, Engine::RPipelineLayout* layout) override;
		Engine::RDescriptorSet* GetDescriptorSet()const { return m_TransformDescs; }
		~RenderMesh();
	};
}