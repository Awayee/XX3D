#include "EditorUI/Public/EditorUIMgr.h"
#include "Resource/Public/Config.h"
#include "Window/Public/EngineWindow.h"
#include "RHI/Public/ImGuiRHI.h"

namespace Editor {

	EditorUIMgr::EditorUIMgr() {
	}

	EditorUIMgr::~EditorUIMgr() {
	}

	void EditorUIMgr::Tick() {
		ImGuiRHI::Instance()->FrameBegin();

		// title bar
		const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(mainViewport->WorkPos, ImGuiCond_Always);
		USize2D windowSize = Engine::EngineWindow::Instance()->GetWindowSize();
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
		for (uint32 i = 0; i < m_Widgets.Size();) {
			auto& wnd = m_Widgets[i];
			if(wnd->m_ToDelete) {
				m_Widgets.SwapRemoveAt(i);
			}
			else {
				++i;
				wnd->Update();
				wnd->Display();
			}
		}
		ImGuiRHI::Instance()->FrameEnd();
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

	EditorWndBase* EditorUIMgr::AddWindow(const char* name, Func<void()>&& func, ImGuiWindowFlags flags) {
		TUniquePtr<EditorFuncWnd> wndPtr(new EditorFuncWnd(name, func, flags));
		EditorWndBase* ptr = wndPtr.Get();
		m_Widgets.PushBack(MoveTemp(wndPtr));
		return ptr;
	}

	void EditorUIMgr::AddWindow(TUniquePtr<EditorWndBase>&& wnd) {
		m_Widgets.PushBack(MoveTemp(wnd));
	}

	void EditorUIMgr::DeleteWindow(EditorWndBase*& pWnd) {
		pWnd->m_ToDelete = true;
		pWnd = nullptr;
	}

	EditorPopup* EditorUIMgr::AddPopUp(Func<void()>&& func) {
		TUniquePtr<EditorPopup> popPtr(new EditorPopup(func));
		EditorPopup* ptr = popPtr.Get();
		m_Widgets.PushBack(MoveTemp(popPtr));
		return ptr;
	}
}