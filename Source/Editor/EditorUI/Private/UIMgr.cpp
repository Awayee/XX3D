#include "EditorUI/Public/UIMgr.h"
#include "Core/Public/macro.h"
#include "Resource/Public/Config.h"
#include "EditorUI/Public/Widget.h"
#include "EditorUI/Public/EditorWindow.h"
#include "Objects/Public/EngineContext.h"
#include "AssetsBrowser.h"
#include "ViewportWindow.h"
#include "Core/Public/Func.h"

namespace Editor {

	UIMgr::UIMgr() {
	}

	UIMgr::~UIMgr() {
	}

	void UIMgr::AddMenu(const char* barName, const char* name, Func<void()>&& func, bool* pToggle) {
		for(auto& column: m_MenuBar) {
			if(column.Name == barName) {
				column.Items.PushBack({name, func, pToggle });
				return;
			}
		}
		m_MenuBar.PushBack({ barName , {{ name, func, pToggle }}});
	}

	void UIMgr::AddWindow(const char* name, Func<void()>&& func, ImGuiWindowFlags flags) {
		m_Windows.PushBack(MakeUniquePtr<EditorFuncWnd>(name, func, flags));
	}

	void UIMgr::AddWindow(TUniquePtr<EditorWndBase>&& wnd) {
		m_Windows.PushBack(std::forward<TUniquePtr<EditorWndBase>>(wnd));
	}

	void UIMgr::Tick() {
		Engine::ImGuiNewFrame();

		// title bar
		const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(mainViewport->WorkPos, ImGuiCond_Always);
		USize2D windowSize = Engine::Context()->Window()->GetWindowSize();
		ImGui::SetNextWindowSize(ImVec2((float)windowSize.w, (float)windowSize.h), ImGuiCond_Always);
		ImGuiWindowFlags   window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
			ImGuiConfigFlags_NoMouseCursorChange | ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Editor menu", nullptr, window_flags);
		ImGui::PopStyleVar();

		//tick menus
		if(ImGui::BeginMenuBar()) {
			for (auto& column : m_MenuBar) {
				if (ImGui::BeginMenu(column.Name.c_str())) {
					for (auto& item : column.Items) {
						if (ImGui::MenuItem(item.Name.c_str(), nullptr, item.Toggle) && item.Func) {
							item.Func();
						}
					}
				}
				ImGui::EndMenu();
			}
		}
		ImGui::EndMenuBar();
		ImGui::End();

		// dock
		ImGuiID main_docking_id = ImGui::GetID("Main Docking");
		ImGuiDockNodeFlags docFlags = ImGuiDockNodeFlags_None;
		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, { 0.0f, 0.0f, 0.0f, 0.0f });
		ImGui::DockSpace(main_docking_id, { 0, 0 }, docFlags);
		ImGui::PopStyleColor();

		//tick windows
		for(auto& widget: m_Windows) {
			widget->Update();
			if(widget->m_Enable) {
				if(ImGui::Begin(widget->m_Name, &widget->m_Enable, widget->m_Flags)) {
					widget->Display();
				}
				ImGui::End();
			}
		}

		Engine::ImGuiEndFrame();
	}
}
