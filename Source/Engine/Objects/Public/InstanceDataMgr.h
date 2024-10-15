#pragma once
#include "Math/Public/Transform.h"
#include "Math/Public/Geometry.h"
#include "Core/Public/TArray.h"

namespace Object {
	class InstanceDataMgr {
	public:
		struct ClusterNode {
			uint32 InstanceStart;
			uint32 InstanceEnd;
			uint32 ChildStart;
			uint32 ChildEnd;
			Math::AABB3 AABB;
			bool HasChild() const;
		};
		InstanceDataMgr() = default;
		~InstanceDataMgr() = default;
		void Reset();
		void Build(const Math::AABB3& resAABB, TArray<Math::FTransform>&& transforms);
		void Cull(const Math::Frustum& frustum, TArray<Math::FMatrix4x4>& outData);
	private:
		TArray<Math::FTransform> m_Transforms;
		TArray<Math::AABB3> m_AABBs;
		TArray<ClusterNode> m_ClusterNodes;
		void RecursivelyCull(uint32 nodeIndex, const Math::Frustum& frustum, TArray<Math::FMatrix4x4>& outData);
	};
}