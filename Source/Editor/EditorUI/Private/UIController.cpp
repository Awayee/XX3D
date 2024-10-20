#include "EditorUI/Public/UIController.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/EditorConfig.h"
#include "Window/Public/EngineWindow.h"
#include "WndViewport.h"
#include "WndAssetBrowser.h"
#include "WndDetails.h"
#include "WndLevelHierarchy.h"
#include "WndLevelSetting.h"
#include "WndDebugView.h"
#include "WndRenderGraph.h"
#include "System/Public/Configuration.h"
#include "Objects/Public/StaticMesh.h"

namespace Editor {
	void SetImGuiStyle() {
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;
		//float h[3] = { 0.000f, 0.400f, 0.700f };// highlight color
		float h[3] = { 0.600f, 0.600f, 0.600f };// highlight color

		colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
		colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(h[0], h[1], h[2], 1.000f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(h[0], h[1], h[2], 1.000f);
		colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
		colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(h[0], h[1], h[2], 1.000f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(h[0], h[1], h[2], 1.000f);
		colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
		colors[ImGuiCol_TabActive] = ImVec4(0.295f, 0.295f, 0.295f, 1.000f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_DockingPreview] = ImVec4(h[0], h[1], h[2], 0.781f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(h[0], h[1], h[2], 1.000f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(h[0], h[1], h[2], 1.000f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(h[0], h[1], h[2], 1.000f);
		colors[ImGuiCol_NavHighlight] = ImVec4(h[0], h[1], h[2], 1.000f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(h[0], h[1], h[2], 1.000f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

		style.ChildRounding = 4.0f;
		style.FrameBorderSize = 1.0f;
		style.FrameRounding = 2.0f;
		style.GrabMinSize = 7.0f;
		style.PopupRounding = 2.0f;
		style.ScrollbarRounding = 12.0f;
		style.ScrollbarSize = 13.0f;
		style.TabBorderSize = 1.0f;
		style.TabRounding = 0.0f;
		style.WindowRounding = 4.0f;
	}

	void EditorExit(){
		Engine::EngineWindow::Instance()->Close();
	}

	void SaveLayout() {
		const XString path = EditorConfig::GetImGuiConfigSavePath();
		ImGui::SaveIniSettingsToDisk(path.c_str());
		LOG_INFO("ImGui settings saved to %s!", path.c_str());
	}

	void UIController::InitializeImGuiConfig() {
		// imgui
		FSize2D scale = Engine::EngineWindow::Instance()->GetWindowContentScale();
		float contentScale = Math::Max(1.0f, Math::Max(scale.w, scale.h));

		// load font for imgui
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		const XString fontFile = Engine::EngineConfig::Instance().GetEngineAssetDir().append(Engine::EngineConfig::Instance().GetDefaultFont()).string();
		io.Fonts->AddFontFromFileTTF(fontFile.c_str(), contentScale * 16, nullptr, nullptr);
		ASSERT(io.Fonts->Build(), "Failed to build fonts");
		io.IniFilename = nullptr; // Do not save settings
		// Load imgui.ini
		ImGui::LoadIniSettingsFromDisk(EditorConfig::GetImGuiConfigSavePath().c_str());

		SetImGuiStyle();
	}

	void EditorConfigSetting() {
		EditorUIMgr::Instance()->AddWindow("Config", []() {
			ImGui::SliderFloat("Camera Move Speed", EditorConfig::SetCameraMoveSpeed(), 0.001f, 1.0f);
			ImGui::SliderFloat("Camera Rotate Speed", EditorConfig::SetCameraRotateSpeed(), 0.001f, 1.0f);
			ImGui::Checkbox("Display FPS", EditorConfig::SetDisplayFPS());
		});
	}

	UIController::UIController() {
		EditorUIMgr* uiMgr = EditorUIMgr::Instance();

		uiMgr->AddMenuBar("Menu");
		uiMgr->AddMenuBar("Level");
		uiMgr->AddMenuBar("Window");

		//windows
		uiMgr->AddWindow(TUniquePtr(new WndAssetBrowser()));
		uiMgr->AddWindow(TUniquePtr(new WndViewport()));
		uiMgr->AddWindow(TUniquePtr(new WndLevelSetting()));
		uiMgr->AddWindow(TUniquePtr(new WndLevelHierarchy()));
		uiMgr->AddWindow(TUniquePtr(new WndDetails()));
		uiMgr->AddWindow(TUniquePtr(new WndDebugView()));
		uiMgr->AddWindow(TUniquePtr(new WndRenderGraph()));
		uiMgr->AddMenu("Menu", "Config", EditorConfigSetting, nullptr);
		uiMgr->AddMenu("Menu", "Save Layout", SaveLayout, nullptr);
		uiMgr->AddMenu("Menu", "Exit", EditorExit, nullptr);
		uiMgr->AddMenu("Render", "Capture Frame", []() {RHI::Instance()->CaptureFrame(); }, nullptr);
		uiMgr->AddMenu("Render", "Enable GPU Driven", {}, &Object::InstancedMeshRenderSystem::EnableGPUDriven);
	}

	UIController::~UIController() {

	}

	void UIController::Tick() {
	}
}
