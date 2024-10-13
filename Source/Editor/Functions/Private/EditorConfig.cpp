#include "Functions/Public/EditorConfig.h"
#include "System/Public/Configuration.h"
namespace Editor {
	EditorConfig EditorConfig::s_Instance{};

	XString EditorConfig::GetImGuiConfigSavePath() {
		return Engine::EngineConfig::Instance().GetProjectDir().append("imgui.ini").string();
	}
}
