#pragma once
#include "WindowSystem.h"
#include <GLFW/glfw3.h>
#include <unordered_map>


namespace Engine {
	class WindowSystemGLFW final: public WindowSystemBase {
	private:
		static std::unordered_map<int, EKeyCode> s_GLFWKeyCodeMap;
		static std::unordered_map<int, EButtonCode> s_GLFWButtonCodeMap;
		static std::unordered_map<int, EInputType> s_GLFWInputMap;

		GLFWwindow* m_Window {nullptr};
		bool m_FocusMode {false};

		std::vector<OnKeyFunc>         m_OnKeyFunc;
		std::vector<OnMouseButtonFunc> m_OnMouseButtonFunc;
		std::vector<OnCursorPosFunc>   m_OnCursorPosFunc;
		std::vector<OnCursorEnterFunc> m_OnCursorEnterFunc;
		std::vector<OnScrollFunc>      m_OnScrollFunc;
		std::vector<OnDropFunc>        m_OnDropFunc;
		std::vector<OnWindowSizeFunc>  m_OnWindowSizeFunc;
		std::vector<OnWindowCloseFunc> m_OnWindowCloseFunc;
	public:
		WindowSystemGLFW() = default;
		~WindowSystemGLFW();
		void Initialize(const WindowInitInfo& initInfo) override;
		void PollEvents() override;
		bool ShouldClose() override;
		void Close() override;
		void SetTitle(const char* title) override;
		void GetWindowSize(int* w, int* h) override;
		void SetFocusMode(bool focusMode) override;
		bool GetFocusMode()override;
		void* GetWindowHandle()override;
		void GetWindowContentScale(float* x, float* y)override;

		void SetWindowIcon(int count, const WindowIcon* icons)override;

		// register func
		void RegisterOnKeyFunc(OnKeyFunc func)override;
		void RegisterOnMouseButtonFunc(OnMouseButtonFunc func)override;
		void RegisterOnCursorPosFunc(OnCursorPosFunc func)override;
		void RegisterOnCursorEnterFunc(OnCursorEnterFunc func)override;
		void RegisterOnScrollFunc(OnScrollFunc func)override;
		void RegisterOnWindowSizeFunc(OnWindowSizeFunc func)override;
		

	private:
		void InitKeyButtonCodeMap(); // 注册keycode枚举映射
		void InitEvents(); // 注册所有事件
		static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
		static void OnCursorPos(GLFWwindow* window, double x, double y);
		static void OnCusorEnterFunc(GLFWwindow* window, int entered);
		static void OnScroll(GLFWwindow* window, double x, double y);
		static void OnWindowSize(GLFWwindow* window, int width, int height);
	};
}