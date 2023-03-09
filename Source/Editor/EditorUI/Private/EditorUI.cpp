#include "EditorUI/Public/EditorUI.h"
#include "Context/Public/EditorContext.h"
#include "Core/Public/macro.h"
#include "Resource/Public/Config.h"
#include "Resource/Public/Assets.h"
#include "EditorUI/Public/EditorWindow.h"
#include "Objects/Public/EngineContext.h"

namespace Editor {
	void EWindowBase::Run() {
		Update();
		if(m_Show) {
			PreWindow();
			if(ImGui::Begin(m_Name, &m_Show, m_Flags)) {
				OnWindow();
			}
			ImGui::End();
			PostWindow();
		}
	}

	void UIMgr::InitWindows()
	{
		m_Windows.push_back(MakeUniquePtr<ObjectsWindow>());
		m_Windows.push_back(MakeUniquePtr<AssetsWindow>());
		m_Windows.push_back(MakeUniquePtr<DetailWindow>());
		m_Windows.push_back(MakeUniquePtr<ViewportWindow>());
		m_Windows.push_back(MakeUniquePtr<FPSWindow>());
	}

	UIMgr::UIMgr() {
		// imgui
		IMGUI_CHECKVERSION();
		m_Context = ImGui::CreateContext();

		float scaleX, scaleY;
		Engine::Context()->Window()->GetWindowContentScale(&scaleX, &scaleY);
		float contentScale = fmaxf(1.0f, fmaxf(scaleX, scaleY));

		// load font for imgui
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigWindowsMoveFromTitleBarOnly = true;
		auto fontAsset = Engine::Assets::LoadAsset<Engine::AFontAsset>(Engine::GetConfig()->GetDefaultFontPath());
		io.Fonts->AddFontFromFileTTF(fontAsset.Path.c_str(), contentScale * 16, nullptr, nullptr);
		ASSERT(io.Fonts->Build(), "Failed to build fonts");
		//io.IniFilename = nullptr; // Do not save settings

		InitWindows();
	}

	UIMgr::~UIMgr() {
		Engine::ImGuiRelease();
		ImGui::DestroyContext(m_Context); // todo solve
	}

	void UIMgr::Tick() {
		Engine::ImGuiNewFrame();

		// title bar
		const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(mainViewport->WorkPos, ImGuiCond_Always);
		ISize2D windowSize;
		Engine::Context()->Window()->GetWindowSize(&windowSize.w, &windowSize.h);
		ImGui::SetNextWindowSize(ImVec2((float)windowSize.w, (float)windowSize.h), ImGuiCond_Always);
		ImGuiWindowFlags   window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
			ImGuiConfigFlags_NoMouseCursorChange | ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Editor menu", nullptr, window_flags);
		ImGui::PopStyleVar();
		// dock
		ImGuiID main_docking_id = ImGui::GetID("Main Docking");
		ImGuiDockNodeFlags docFlags = ImGuiDockNodeFlags_None;
		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, { 0.0f, 0.0f, 0.0f, 0.0f });
		ImGui::DockSpace(main_docking_id, { 0, 0 }, docFlags);
		ImGui::PopStyleColor();
		// menus
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Menu")) {
				if (ImGui::MenuItem("Save")) {
					LOG("Saved!");
				}
				if (ImGui::MenuItem("Exit")) {
					Engine::Context()->Window()->Close();//close the window
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window")) {
				for(auto& w: m_Windows) {
					ImGui::MenuItem(w->m_Name, nullptr, &w->m_Show);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		ImGui::End();

		for(auto& w: m_Windows) {
			w->Run();
		}

		Engine::ImGuiEndFrame();
	}
}
