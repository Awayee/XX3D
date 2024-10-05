#include "UIExtent.h"
#include "EditorUI/Public/EditorWindow.h"
#include "Functions/Public/AssetManager.h"
#ifdef _WIN32
#include <Windows.h>
#endif
namespace ImGui {
	XString OpenFileDialog(const char* filter) {
#ifdef _WIN32
		XString filename; filename.resize(128);
		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = NULL;
		ofn.lpstrFile = filename.data();
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = (DWORD)filename.size();
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 0;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		if (GetOpenFileName(&ofn) == TRUE) {
			return filename;
		}
#endif
		return "";
	}

	void OpenFileExplorer(const char* path) {
		ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOWDEFAULT);
	}

	bool DraggableFileItemAssets(const char* name, XString& file, const char* filter) {
		ImGui::InputText(name, file.data(), file.size(), ImGuiInputTextFlags_ReadOnly);
		bool isSrcDirty = false;
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("File")) {
				const Editor::FileNode* fileNode = (const Editor::FileNode*)payload->Data;
				if (StrContains(filter, fileNode->GetExt().c_str())) {
					file = fileNode->GetPathStr();
					isSrcDirty = true;
				}
			}
			ImGui::EndDragDropTarget();
		}
		return isSrcDirty;
	}

	bool DraggableFileItemGlobal(const char* name, XString& file, const char* filter) {
		bool isSrcDirty = false;
		if (ImGui::Button(name)) {
			file = OpenFileDialog(filter);
			isSrcDirty = true;
		}
		ImGui::SameLine();
		ImGui::InputText("##", file.data(), file.size(), ImGuiInputTextFlags_ReadOnly);
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("File")) {
				const Editor::FileNode* fileNode = (const Editor::FileNode*)payload->Data;
				if (StrContains(filter, fileNode->GetExt().c_str())) {
					file = fileNode->GetFullPath().string();
					isSrcDirty = true;
				}
			}
			ImGui::EndDragDropTarget();
		}
		return isSrcDirty;
	}
}
