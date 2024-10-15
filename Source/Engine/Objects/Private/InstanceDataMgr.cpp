#include "Objects/Public/InstanceDataMgr.h"
#include "Math/Public/Math.h"

namespace Object {

	inline uint32 GetVector3MaxAxis(const Math::FVector3& vec) {
		return vec.X < vec.Y ? (vec.Y < vec.Z ? 2 : 1) : (vec.X < vec.Z ? 2 : 0);
	}

	static constexpr uint32 NUM_LEAF_INSTANCE_MAX = 16;
	static constexpr uint32 NUM_CHILD_NODE_MAX = 8;
	static constexpr uint32 INVALID_INDEX = UINT32_MAX;

	class ClusterTreeBuilder {
		using ClusterNode = InstanceDataMgr::ClusterNode;
	public:
		TArray<ClusterNode> ClusterNodes;
		ClusterTreeBuilder(TArray<Math::FTransform>& transforms, TArray<Math::AABB3>& aabbs): m_RefTransforms(transforms), m_RefAABBs(aabbs) {
			ASSERT(m_RefTransforms.Size() == m_RefAABBs.Size(), "transforms is not matches aabbs!");
			if(transforms.IsEmpty()) {
				return;
			}
			TArray<uint32> indirectIndices(m_RefAABBs.Size());
			for(uint32 i=0; i<indirectIndices.Size(); ++i) {
				indirectIndices[i] = i;
			}

			// 1. build leaf nodes
			TArray<TArray<ClusterNode>> treeLayers;
			auto& leafNodes = treeLayers.EmplaceBack();
			RecursivelySeparateClusters(indirectIndices, 0, indirectIndices.Size(), leafNodes);

			// 2. build other layers of tree
			uint32 layerNodeSize = leafNodes.Size();
			uint32 nodeSize = layerNodeSize + 1; // with root node
			while(layerNodeSize > NUM_CHILD_NODE_MAX) {
				auto& newLayer = treeLayers.EmplaceBack();
				auto& childLayer = treeLayers[treeLayers.Size() - 2];
				RecursivelyBuildClusterTreeLayer(childLayer, 0, childLayer.Size(), newLayer);
				layerNodeSize = newLayer.Size();
				nodeSize += layerNodeSize;
			}
			//  record index start after pushed.
			nodeSize = 1; // with root node.
			TArray<uint32> layerIndexEnds(treeLayers.Size());
			for(int i=(int)treeLayers.Size()-1; i>-1; --i) {
				nodeSize += treeLayers[i].Size();
				layerIndexEnds[i] = nodeSize;
			}

			// 3. reorder instances
			TArray<uint32> orderMap(indirectIndices.Size());
			uint32 instanceIndex = 0;
			for(auto& node : treeLayers[0]) {
				const uint32 newInstanceStart = instanceIndex;
				for(uint32 i=node.InstanceStart; i<node.InstanceEnd; ++i) {
					orderMap[indirectIndices[i]] = instanceIndex++;
				}
				node.InstanceStart = newInstanceStart;
				node.InstanceEnd = instanceIndex;
			}
			for(uint32 i=0; i<orderMap.Size(); ++i) {
				while(orderMap[i] != i) {
					const uint32 targetIdx = orderMap[i];
					m_RefAABBs.SwapEle(i, targetIdx);
					m_RefTransforms.SwapEle(i, targetIdx);
					orderMap.SwapEle(i, targetIdx);
				}
			}

			// 4. reorder tree nodes
			uint32 nodeIndex = 0;
			for(uint32 i=1; i<treeLayers.Size(); ++i) {
				auto& childLayer = treeLayers[i - 1];
				auto& currentLayer = treeLayers[i];
				for(auto& node: currentLayer) {
					for(uint32 j=node.ChildStart; j<node.ChildEnd; ++j) {
						const auto& childNode = childLayer[j];
						node.InstanceStart = Math::Min(node.InstanceStart, childNode.InstanceStart);
						node.InstanceEnd = Math::Max(node.InstanceEnd, childNode.InstanceEnd);
					}
					// offset children indices for keep consistent with indices pushed
					node.ChildStart += layerIndexEnds[i];
					node.ChildEnd += layerIndexEnds[i];
				}
			}

			// 5. push nodes
			ClusterNodes.Reserve(nodeSize);
			//   add root node
			auto& childLayer = treeLayers.Back();
			auto& root = ClusterNodes.EmplaceBack();
			root.ChildStart = 1;
			root.ChildEnd = root.ChildStart + childLayer.Size();
			root.InstanceStart = UINT32_MAX;
			root.InstanceEnd = 0;
			uint32 idx = 0;
			root.AABB = childLayer[idx].AABB;
			for(; idx <childLayer.Size(); ++idx) {
				auto& childNode = childLayer[idx];
				root.InstanceStart = Math::Min(root.InstanceStart, childNode.InstanceStart);
				root.InstanceEnd = Math::Max(root.InstanceEnd, childNode.InstanceEnd);
				root.AABB.Union(childNode.AABB);
			}
			//  add children
			for(; treeLayers.Size(); treeLayers.PopBack()) {
				ClusterNodes.PushBack(treeLayers.Back());
			}
		}

	private:
		TArray<Math::FTransform>& m_RefTransforms;
		TArray<Math::AABB3>& m_RefAABBs;
		void RecursivelySeparateClusters(TArray<uint32>& indirectIndices, uint32 start, uint32 end, TArray<ClusterNode>& outNodes) {
			// calc entire AABB
			uint32 i = start;
			Math::AABB3 entireAABB = m_RefAABBs[indirectIndices[i++]];
			for (; i < end; ++i) {
				auto& instanceAABB = m_RefAABBs[indirectIndices[i]];
				entireAABB.Union(instanceAABB);
			}

			if(end - start > NUM_LEAF_INSTANCE_MAX) {
				// separate elements on max axis
				const auto extent = entireAABB.Extent();
				const uint32 axis = GetVector3MaxAxis(extent);
				indirectIndices.Sort(start, end, [this, axis](uint32 l, uint32 r) {
					return m_RefAABBs[l].Center()[axis] < m_RefAABBs[r].Center()[axis];
				});
				const uint32 middle = (start + end) / 2;
				RecursivelySeparateClusters(indirectIndices, start, middle, outNodes);
				RecursivelySeparateClusters(indirectIndices, middle, end, outNodes);
			}
			else {
				// push leaf cluster node
				ClusterNode& node = outNodes.EmplaceBack();
				node.InstanceStart = start;
				node.InstanceEnd = end;
				node.ChildStart = node.ChildEnd = INVALID_INDEX;
				node.AABB = entireAABB;
			}
		}

		void RecursivelyBuildClusterTreeLayer(TArray<ClusterNode>& clusters, uint32 start, uint32 end, TArray<ClusterNode>& outNodes) {
			// calc entire AABB
			uint32 i = start;
			Math::AABB3 entireAABB = clusters[i++].AABB;
			for (; i < end; ++i) {
				entireAABB.Union(clusters[i].AABB);
			}

			if(end - start > NUM_CHILD_NODE_MAX) {
				// separate elements on max axis
				const auto extent = entireAABB.Extent();
				const uint32 axis = GetVector3MaxAxis(extent);
				clusters.Sort(start, end, [axis](const ClusterNode& l, const ClusterNode& r) {
					return l.AABB.Center()[axis] < r.AABB.Center()[axis];
				});
				const uint32 middle = (start + end) / 2;
				RecursivelyBuildClusterTreeLayer(clusters, start, middle, outNodes);
				RecursivelyBuildClusterTreeLayer(clusters, middle, end, outNodes);
			}
			else {
				ClusterNode& node = outNodes.EmplaceBack();
				node.InstanceStart = UINT32_MAX;
				node.InstanceEnd = 0;
				node.ChildStart = start;
				node.ChildEnd = end;
				node.AABB = entireAABB;
			}
		}
	};

	bool InstanceDataMgr::ClusterNode::HasChild() const {
		return INVALID_INDEX != ChildStart && INVALID_INDEX != ChildEnd;
	}

	void InstanceDataMgr::Reset() {
		m_Transforms.Reset();
		m_AABBs.Reset();
		m_ClusterNodes.Reset();
	}

	void InstanceDataMgr::Build(const Math::AABB3& resAABB, TArray<Math::FTransform>&& transforms) {
		Reset();
		m_Transforms = MoveTemp(transforms);
		m_AABBs.Reserve(m_Transforms.Size());
		for(auto& transform: m_Transforms) {
			m_AABBs.PushBack(resAABB.Transform(transform.ToMatrix()));
		}
		ClusterTreeBuilder clusterTreeBuilder(m_Transforms, m_AABBs);
		m_ClusterNodes.Swap(clusterTreeBuilder.ClusterNodes);
	}

	void InstanceDataMgr::Cull(const Math::Frustum& frustum, TArray<Math::FMatrix4x4>& outData) {
		RecursivelyCull(0, frustum, outData);
	}

	void InstanceDataMgr::RecursivelyCull(uint32 nodeIndex, const Math::Frustum& frustum, TArray<Math::FMatrix4x4>& outData) {
		auto& node = m_ClusterNodes[nodeIndex];
		if(frustum.Cull(node.AABB)) {
			if(node.HasChild()) {
				for(uint32 i=node.ChildStart; i<node.ChildEnd; ++i) {
					RecursivelyCull(i, frustum, outData);
				}
			}
			else {
				for(uint32 i=node.InstanceStart; i<node.InstanceEnd; ++i) {
					if(frustum.Cull(m_AABBs[i])) {
						outData.PushBack(m_Transforms[i].ToMatrix());
					}
				}
			}
		}
	}
}
