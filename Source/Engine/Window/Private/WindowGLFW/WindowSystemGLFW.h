#pragma once
#include "Core/Public/Container.h"
#include "Core/Public/TArray.h"
#include "Window/Public/EngineWindow.h"
#include <GLFW/glfw3.h>
#include <unordered_map>


namespace Engine {
	class WindowSystemGLFW final: public EngineWindow {
	public:
		explicit WindowSystemGLFW(const WindowInitInfo& initInfo);
		~WindowSystemGLFW() override;
		void Update() override;
		void Close() override;
		void SetTitle(const char* title) override;
		void SetFocusMode(bool focusMode) override;
		void SetWindowIcon(int count, const WindowIcon* icons)override;
		bool GetFocusMode()override;
		void* GetWindowHandle()override;
		USize2D GetWindowSize() override;
		FSize2D GetWindowContentScale() override;
		FOffset2D GetCursorPos() override;

		// register func
		void RegisterOnKeyFunc(OnKeyFunc&& func)override;
		void RegisterOnMouseButtonFunc(OnMouseButtonFunc&& func)override;
		void RegisterOnCursorPosFunc(OnCursorPosFunc&& func)override;
		void RegisterOnCursorEnterFunc(OnCursorEnterFunc&& func)override;
		void RegisterOnScrollFunc(OnScrollFunc&& func)override;
		void RegisterOnWindowSizeFunc(OnWindowSizeFunc&& func)override;

	private:
		static TUnorderedMap<int, EKey> s_GLFWKeyCodeMap;
		static TUnorderedMap<int, EBtn> s_GLFWButtonCodeMap;
		static TUnorderedMap<int, EInput> s_GLFWInputMap;

		GLFWwindow* m_Window {nullptr};
		USize2D m_Size;
		bool m_FocusMode {false};
		TArray<OnKeyFunc>         m_OnKeyFunc;
		TArray<OnMouseButtonFunc> m_OnMouseButtonFunc;
		TArray<OnCursorPosFunc>   m_OnCursorPosFunc;
		TArray<OnCursorEnterFunc> m_OnCursorEnterFunc;
		TArray<OnScrollFunc>      m_OnScrollFunc;
		TArray<OnDropFunc>        m_OnDropFunc;
		TArray<OnWindowSizeFunc>  m_OnWindowSizeFunc;
		TArray<OnWindowCloseFunc> m_OnWindowCloseFunc;
		TArray<OnWindowFocus>     m_OnWindowFocusFunc;
		void InitKeyButtonCodeMap();
		void InitEvents();
		static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
		static void OnCursorPos(GLFWwindow* window, double x, double y);
		static void OnCursorEnter(GLFWwindow* window, int entered);
		static void OnScroll(GLFWwindow* window, double x, double y);
		static void OnWindowSize(GLFWwindow* window, int width, int height);
		static void OnWindowFocus(GLFWwindow* window, int focus);
	};
}