#include "WndRenderGraph.h"
#include "Render/Public/Renderer.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Math/Public/Math.h"
#include <imgui.h>
#include <imnodes.h>

namespace Editor {

    using RGViewNode = Render::RenderGraphView::Node;

	void RecursivelyBuildPinLinks(
		const TArray<RGViewNode>& nodes,
		Render::RGNodeID nodeID,
		TArray<TPair<int, int>>& pinIDs,
		TArray<TPair<int, int>>& lines,
		TArray<bool>& visited,
		int& uniqueID)
	{
		if(!visited[nodeID]) {
			auto& node = nodes[nodeID];
			if(node.PrevNodes.Size()) {
				if(-1 == pinIDs[nodeID].first) {
					pinIDs[nodeID].first = uniqueID++;
				}
				for(const auto prevID: node.PrevNodes) {
					if(-1 == pinIDs[prevID].second) {
						pinIDs[prevID].second = uniqueID++;
					}
					RecursivelyBuildPinLinks(nodes, prevID, pinIDs, lines, visited, uniqueID);
					lines.PushBack({ pinIDs[prevID].second, pinIDs[nodeID].first });
				}
			}
			visited[nodeID] = true;
		}
	}

	// build positions of nodes, reference https://juejin.cn/post/7038879014118752264
	class RGNodeViewLayoutBuilder {
	public:
		TArray<ImVec2> Positions;
		RGNodeViewLayoutBuilder(const TArray<RGViewNode>& nodes, Render::RGNodeID targetID) : m_RefNodes(nodes) {
			if(m_RefNodes.IsEmpty() || Render::RG_INVALID_NODE == targetID) {
				return;
			}
			Positions.Resize(nodes.Size());
			m_TempNodes.Resize(nodes.Size());
			// dfs to get layers of nodes
			m_Visited.Resize(nodes.Size(), false);
			RecursivelyBuildNodeLayers(targetID, 0);
			// again to get y pos of nodes
			m_Visited.Reset(); m_Visited.Resize(nodes.Size(), false);
			RecursivelyBuildNode(targetID,0.0f);
			// build positions
			TArray<float> layerPosXs(m_LayerMaxWidths.Size(), 0.0f);
			for(int i=(int)layerPosXs.Size()-2; i>-1; --i) {
				layerPosXs[i] += layerPosXs[i+1] + m_LayerMaxWidths[i+1] + s_XPadding;
			}
			for(uint32 i=0; i<nodes.Size(); ++i) {
				const auto& tempData = m_TempNodes[i];
				float x = layerPosXs[tempData.Layer] + (m_LayerMaxWidths[tempData.Layer] - tempData.NodeSize.x) * 0.5f;// align x to center
				float y = tempData.ExtentY.Min;
				Positions[i] = { x,y };
			}
		}
	private:
		struct Extent {
			float Min{ 0.0f }, Max{ 0.0f };
			Extent& Union(const Extent& rhs) {
				Min = Math::Min(Min, rhs.Min);
				Max = Math::Max(Max, rhs.Max);
				return *this;
			}
		};
		struct NodeTempData {
			uint32 Layer;
			ImVec2 NodeSize;
			Extent ExtentY;
			Extent ExtentYWithChildren;
		};
		const TArray<RGViewNode>& m_RefNodes;
		TArray<NodeTempData> m_TempNodes;
		TArray<bool> m_Visited; // whether node is visited
		TArray<float> m_LayerMaxWidths;
		int m_PinIDMax{ 0u };
		static constexpr float s_XPadding = 32.0f, s_YPadding = 16.0f;

		void TrySetLayerMaxWidth(uint32 layer, float width) {
			if(m_LayerMaxWidths.Size() <= layer) {
				m_LayerMaxWidths.Resize(layer + 1);
				m_LayerMaxWidths[layer] = width;
			}
			else {
				m_LayerMaxWidths[layer] = Math::Max(m_LayerMaxWidths[layer], width);
			}
		}

		// sort layers, get max width per layer, get pins and lines
		void RecursivelyBuildNodeLayers(Render::RGNodeID nodeID, uint32 layer) {
			auto& node = m_RefNodes[nodeID];
			auto& temp = m_TempNodes[nodeID];
			ImVec2 nodeSize;
			if(!m_Visited[nodeID]) {
				nodeSize = ImNodes::GetNodeDimensions((int)nodeID);
				temp.NodeSize = nodeSize;
				temp.Layer = layer;
				m_Visited[nodeID] = true;
			}
			else {
				nodeSize = temp.NodeSize;
				temp.Layer = Math::Max(temp.Layer, layer);
			}
			TrySetLayerMaxWidth(layer, nodeSize.x);
			for(const Render::RGNodeID prevID: node.PrevNodes) {
				RecursivelyBuildNodeLayers(prevID, layer + 1);
			}
		}

		// build y pos, return y extent with children of the node.
		Extent RecursivelyBuildNode(Render::RGNodeID nodeID, float yStart) {
			auto& temp = m_TempNodes[nodeID];
			// if visited, return cached value
			if(m_Visited[nodeID]) {
				return temp.ExtentYWithChildren;
			}

			auto& node = m_RefNodes[nodeID];
			Extent extWithChildren{yStart, yStart};
			float childYStart = yStart;
			for (const Render::RGNodeID prevID : node.PrevNodes) {
				Extent ext = RecursivelyBuildNode(prevID, childYStart);
				extWithChildren.Union(ext);
				childYStart = ext.Max + s_YPadding;
			}
			Extent nodeExt;
			nodeExt.Min = (extWithChildren.Min + extWithChildren.Max - temp.NodeSize.y) * 0.5f;
			nodeExt.Min = Math::Max(nodeExt.Min, yStart); // must greater than last brother.
			nodeExt.Max = nodeExt.Min + temp.NodeSize.y;
			temp.ExtentY = nodeExt;
			temp.ExtentYWithChildren = extWithChildren.Union(nodeExt);
			m_Visited[nodeID] = true;
			return extWithChildren;
		}
	};

	WndRenderGraph::WndRenderGraph() : EditorWndBase("Render Graph", ImGuiWindowFlags_HorizontalScrollbar), m_UpdateFrame(UINT32_MAX){
		EditorUIMgr::Instance()->AddMenu("Window", m_Name.c_str(), {}, &m_Enable);
		m_Enable = false;
	}

	void WndRenderGraph::WndContent() {
		if(ImGui::Button("Refresh")) {
			Render::Renderer::Instance()->RefreshRenderGraphView();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Parallel Nodes", &Render::RenderGraph::ParrallelNodes);
		const auto& rgView = Render::Renderer::Instance()->GetRenderGraphView();
		const auto& nodes = rgView.Nodes;
		if(!nodes.Size()) {
			return;
		}
		// check and rebuild data
		const bool needRebuild = UINT32_MAX == m_UpdateFrame || m_UpdateFrame < rgView.UpdateFrame;
		if (needRebuild) {
			m_NodePins.Reset(); m_NodePins.Resize(nodes.Size(), { -1, -1 });
			m_NodeLines.Reset();
			if (nodes.Size()) {
				TArray<bool> visited(nodes.Size(), false);
				int uniqueID = 0;
				RecursivelyBuildPinLinks(nodes, rgView.LastID, m_NodePins, m_NodeLines, visited, uniqueID);
			}
			m_UpdateFrame = rgView.UpdateFrame;
		}

		ImNodes::PushStyleVar(ImNodesStyleVar_PinCircleRadius, 4.0f);
		ImNodes::PushStyleVar(ImNodesStyleVar_PinTriangleSideLength, 10.0f);
		ImNodes::PushStyleVar(ImNodesStyleVar_PinQuadSideLength, 8.0f);
		ImNodes::BeginNodeEditor();
		//DrawNodeViewRecursively2(nodes, pins, rgView.LastID, Render::RG_INVALID_NODE, uniqueID);
		for (uint32 i = 0; i < nodes.Size(); ++i) {
			const auto& node = nodes[i];
			const int nodeID = (int)i;
			float cornerBounding, borderThickness;
			ImColor bgColor, outlineColor;
			if (node.Type == Render::ERGNodeType::Resource) {
				cornerBounding = 8.0f;
				bgColor = { 4,32,4,255 };
			}
			else {
				cornerBounding = 0.0f;
				bgColor = { 4,4,32,255 };
			}
			if (ImNodes::IsNodeSelected(nodeID)) {
				borderThickness = 4.0f;
				outlineColor = { 128, 72, 0, 255 };
			}
			else {
				borderThickness = 2.0f;
				outlineColor = { 64, 64, 64, 255 };
			}
			ImNodes::PushStyleVar(ImNodesStyleVar_NodeCornerRounding, cornerBounding);
			ImNodes::PushStyleVar(ImNodesStyleVar_NodeBorderThickness, borderThickness);
			ImNodes::PushColorStyle(ImNodesCol_NodeBackground, bgColor);
			ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundHovered, 2 * bgColor);
			ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundSelected, 2 * bgColor);
			ImNodes::PushColorStyle(ImNodesCol_NodeOutline, outlineColor);

			ImNodes::BeginNode(nodeID);
			ImGui::TextUnformatted(node.Name.c_str());
			//  draw input pin
			const auto& nodePin = m_NodePins[i];
			if (-1 != nodePin.first) {
				ImNodes::BeginInputAttribute(nodePin.first, ImNodesPinShape_TriangleFilled);
				ImGui::TextUnformatted("In");
				ImNodes::EndInputAttribute();
				ImGui::SameLine();
			}
			//  draw output pin
			if (-1 != nodePin.second) {
				ImNodes::BeginOutputAttribute(nodePin.second, ImNodesPinShape_CircleFilled);
				const float text_width = ImGui::CalcTextSize("Out").x;
				const float nodeWidth = ImGui::CalcTextSize(node.Name.c_str()).x;
				ImGui::Indent(nodeWidth - text_width);
				ImGui::TextUnformatted("Out");
				ImNodes::EndOutputAttribute();
			}
			ImNodes::EndNode();
			ImNodes::PopStyleVar();
			ImNodes::PopColorStyle();
		}
		//lines
		int uniqueID = 0;
		for (const auto& line : m_NodeLines) {
			ImNodes::Link(uniqueID++, line.first, line.second);
		}
		// check rebuild
		if (needRebuild) {
			RGNodeViewLayoutBuilder builder(nodes, rgView.LastID);
			auto& positions = builder.Positions;
			for (uint32 i = 0; i < positions.Size(); ++i) {
				ImNodes::SetNodeGridSpacePos((int)i, positions[i]);
			}
		}
		ImNodes::MiniMap();
		ImNodes::EndNodeEditor();
	}

	void WndRenderGraph::OnOpen() {
		EditorWndBase::OnOpen();
		Render::Renderer::Instance()->RefreshRenderGraphView();
	}
}
