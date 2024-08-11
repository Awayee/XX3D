#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/String.h"
#include "Core/Public/TSingleton.h"

namespace Editor {
	class EditorConfig: public TSingleton<EditorConfig> {
		friend TSingleton<EditorConfig>;
	public:
		const XString& GetImGuiConfigSavePath();
		static void InitializeImGuiConfig();
	private:
		XString m_ImGuiConfigSavePath;
		EditorConfig();
		~EditorConfig() override;
	};
}
