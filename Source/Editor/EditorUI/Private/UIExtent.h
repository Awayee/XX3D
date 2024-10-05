#pragma once
#include "Core/Public/String.h"
#include <imgui.h>
namespace ImGui {


	XString OpenFileDialog(const char* filter);

	void OpenFileExplorer(const char* path);

	bool DraggableFileItemAssets(const char* name, XString& file, const char* filter);

	bool DraggableFileItemGlobal(const char* name, XString& file, const char* filter);

	inline bool ImGuiIsMouseClicked() {
		return ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right);
	}

	inline bool ImGuiIsMouseReleased() {
		return ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right);
	}
}
