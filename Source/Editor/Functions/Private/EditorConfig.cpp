#include "Functions/Public/EditorConfig.h"

namespace Editor {
	EditorConfig EditorConfig::s_Instance{};

	XString EditorConfig::GetImGuiConfigSavePath() {
		XString path{ PROJECT_CONFIG };
		path.append("imgui.ini");
		return path;
	}
}
