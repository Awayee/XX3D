#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/String.h"

namespace Editor {
	class EditorConfig {
		SINGLETON_INSTANCE(EditorConfig);
	public:
		const XString& GetImGuiConfigSavePath();
		static void InitializeImGuiConfig();
	private:
		XString m_ImGuiConfigSavePath;
		EditorConfig();
		~EditorConfig();
	};
}
