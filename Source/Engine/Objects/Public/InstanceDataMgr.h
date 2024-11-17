#pragma once
#include "Math/Public/Transform.h"
#include "Math/Public/Geometry.h"
#include "Core/Public/TArray.h"
#include "RHI/Public/RHI.h"
//#define INSTANCE_HALF_FLOAT // TODO half float is not supported by Nvidia GPU on Vulkan

namespace Object {
	struct HalfMatrix4x4 {
		uint16 Mat[4][4];
		HalfMatrix4x4& operator=(const Math::FMatrix4x4& fMatrix);
	};

	ERHIFormat GetInstanceDataRowFormat();

	struct InstanceData {
#ifdef INSTANCE_HALF_FLOAT
		HalfMatrix4x4 TransformMatrix;
#else
		Math::FMatrix4x4 TransformMatrix;
#endif
	};

	struct InstanceDrawRange {
		uint32 InstanceStart;
		uint32 InstanceEnd;
	};

	class InstanceDataMgr {
	public:
		struct ClusterNode {
			Math::AABB3 AABB;
			uint32 InstanceStart;
			uint32 InstanceEnd;
			uint32 ChildStart;
			uint32 ChildEnd;
			bool HasChild() const;
		};
		InstanceDataMgr();
		~InstanceDataMgr() = default;
		NON_COPYABLE(InstanceDataMgr);
		InstanceDataMgr(InstanceDataMgr&& rhs) noexcept;
		InstanceDataMgr& operator=(InstanceDataMgr&& rhs)noexcept;
		void Reset();
		void Build(const Math::AABB3& resAABB, const TArray<Math::FTransform>& transforms);
		RHIBuffer* GetInstanceBuffer();
		TConstArrayView<InstanceData> GetInstances() const;
		TConstArrayView<Math::AABB3> GetInstanceAABBs() const;
		TConstArrayView<ClusterNode> GetClusters() const;
		bool IsEmpty() const;
		void GenerateInstanceID(const Math::Frustum& frustum, TArray<uint32>& outInstanceIDs);
		void GenerateDrawRanges(const Math::Frustum& frustum, TArray<InstanceDrawRange>& ranges);
	private:
		RHIBufferPtr m_InstanceBuffer;
		TArray<InstanceData> m_Instances;
		TArray<Math::AABB3> m_AABBs;
		TArray<ClusterNode> m_ClusterNodes;
		uint32 m_ClusterSize;
		void RecursivelyGenerateInstanceID(uint32 nodeIndex, const Math::Frustum& frustum, TArray<uint32>& outInstanceIDs);
		void RecursivelyGenerateDrawRanges(uint32 nodeIndex, const Math::Frustum& frustum, TArray<InstanceDrawRange>& ranges);
	};
}