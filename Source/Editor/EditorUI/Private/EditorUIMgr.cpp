#include "EditorUI/Public/EditorUIMgr.h"
#include "Resource/Public/Config.h"
#include "Objects/Public/EngineContext.h"
#include "Window/Public/Wnd.h"

namespace Editor {

	EditorUIMgr::EditorUIMgr() {
	}

	EditorUIMgr::~EditorUIMgr() {
	}

	void EditorUIMgr::Tick() {
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

		//dock
		ImGuiID main_docking_id = ImGui::GetID("Main Docking");
		ImGuiDockNodeFlags docFlags = ImGuiDockNodeFlags_None;
		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, { 0.0f, 0.0f, 0.0f, 1.0f });
		ImGui::DockSpace(main_docking_id, { 0, 0 }, docFlags);
		ImGui::PopStyleColor();

		//menus
		if (ImGui::BeginMenuBar()) {
			for (auto& column : m_MenuBar) {
				if (ImGui::BeginMenu(column.Name.c_str())) {
					for (auto& item : column.Items) {
						if (ImGui::MenuItem(item.Name.c_str(), nullptr, item.Toggle) && item.Func) {
							item.Func();
						}
					}
					ImGui::EndMenu();
				}
			}
		}
		ImGui::EndMenuBar();

		ImGui::End();
		//windows
		for (uint32 i = 0; i < m_Windows.Size();) {
			auto& wnd = m_Windows[i];
			if(wnd->m_ToDelete) {
				m_Windows.SwapRemoveAt(i);
			}
			else {
				++i;
				wnd->Update();
				if(wnd->m_Enable) {
					if(ImGui::Begin(wnd->m_Name, &wnd->m_Enable, wnd->m_Flags)) {
						// support right click to focus window
						if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
							ImGui::SetWindowFocus();
						}
						wnd->Display();
					}
					ImGui::End();
				}
			}
		}

		Engine::ImGuiEndFrame();
	}

	void EditorUIMgr::AddMenuBar(const char* barName) {
		for (auto& column : m_MenuBar) {
			if (column.Name == barName) {
				return;
			}
		}
		m_MenuBar.PushBack({ barName, {} });
	}

	void EditorUIMgr::AddMenu(const char* barName, const char* name, Func<void()>&& func, bool* pToggle) {
		for(auto& column: m_MenuBar) {
			if(column.Name == barName) {
				column.Items.PushBack({name, func, pToggle });
				return;
			}
		}
		m_MenuBar.PushBack({ barName , {{ name, func, pToggle }}});
	}

	EditorWindowBase* EditorUIMgr::AddWindow(const char* name, Func<void()>&& func, ImGuiWindowFlags flags) {
		m_Windows.PushBack(MakeUniquePtr<EditorFuncWnd>(name, func, flags));
		return m_Windows.Back().get();
	}

	void EditorUIMgr::AddWindow(TUniquePtr<EditorWindowBase>&& wnd) {
		m_Windows.PushBack(std::forward<TUniquePtr<EditorWindowBase>>(wnd));
	}

	void EditorUIMgr::DeleteWindow(EditorWindowBase*& pWnd) {
		pWnd->m_ToDelete = true;
		pWnd = nullptr;
	}

	void EditorUIMgr::RemoveWindow(WidgetID wndId) {

	}
}
