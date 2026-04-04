#include "Functions/Public/EditorConfig.h"
#include "Core/Public/File.h"
#include "System/Public/ConfigManager.h"

namespace Editor {
	EditorConfig EditorConfig::s_Instance{};

	XString EditorConfig::GetImGuiConfigSavePath() {
		return File::CombinePathStr(Engine::ConfigMgr::Instance().GetProjectDir(), "imgui.ini");
	}
}
