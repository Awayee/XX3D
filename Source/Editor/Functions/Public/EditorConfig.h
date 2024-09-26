#pragma once
#include "Core/Public/String.h"

namespace Editor {
	class EditorConfig {
	public:
		static XString GetImGuiConfigSavePath();
	private:
#define EDITOR_CONFIG_PROPERTY(Type, Name, DefaultVal)\
	private:\
	Type m_##Name{DefaultVal};\
	public:\
	static Type Get##Name(){return s_Instance.m_##Name;}\
	static Type* Set##Name(){return &s_Instance.m_##Name;}
		static EditorConfig s_Instance;
		EDITOR_CONFIG_PROPERTY(float, CameraMoveSpeed, 0.1f);
		EDITOR_CONFIG_PROPERTY(float, CameraRotateSpeed, 0.1f);
	};
}
