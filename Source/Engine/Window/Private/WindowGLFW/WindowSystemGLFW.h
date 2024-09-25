#pragma once
#include "Core/Public/Container.h"
#include "Core/Public/TArray.h"
#include "Window/Public/EngineWindow.h"
#include "Core/Public/EnumClass.h"
#include <GLFW/glfw3.h>

namespace Engine {
	class WindowSystemGLFW final: public EngineWindow {
	public:
		explicit WindowSystemGLFW(const WindowInitInfo& initInfo);
		~WindowSystemGLFW() override;
		void Update() override;
		void Close() override;
		void SetTitle(const char* title) override;
		void SetWindowIcon(int count, const WindowIcon* icons)override;
		bool GetFocusMode()override;
		void* GetWindowHandle()override;
		USize2D GetWindowSize() override;
		FSize2D GetWindowContentScale() override;
		FOffset2D GetCursorPos() override;
		bool IsKeyDown(EKey key) override;
		bool IsMouseDown(EBtn btn) override;
	private:
		TInputBidirectionalMap2<EKey> m_KeyMap;
		TInputBidirectionalMap<GLFW_MOUSE_BUTTON_1, GLFW_MOUSE_BUTTON_LAST, EBtn> m_MouseButtonMap;
		TInputBidirectionalMap<GLFW_RELEASE, GLFW_REPEAT, EInput> m_InputMap;
		GLFWwindow* m_Window {nullptr};
		USize2D m_Size;
		bool m_FocusMode {false};
		void InitKeyButtonCodeMap();
		void InitEvents();
		static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
		static void OnCursorPos(GLFWwindow* window, double x, double y);
		static void OnScroll(GLFWwindow* window, double x, double y);
		static void OnWindowSize(GLFWwindow* window, int width, int height);
		static void OnWindowFocus(GLFWwindow* window, int focus);
		static void OnDrop(GLFWwindow* window, int pathNum, const char** paths);
	};
}
