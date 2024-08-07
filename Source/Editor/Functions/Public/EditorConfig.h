#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/String.h"
#include "Core/Public/TSingleton.h"

namespace Editor {
	class EditorConfig: public TSingleton<EditorConfig> {
		friend TSingleton<EditorConfig>;
	public:
		const XXString& GetImGuiConfigSavePath();
		static void InitializeImGuiConfig();
	private:
		XXString m_ImGuiConfigSavePath;
		EditorConfig();
		~EditorConfig() override;
	};
}
