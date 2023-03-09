#include "Render/Public/WindowSystemGLFW.h"

namespace Engine {

	std::unordered_map<int, EKeyCode> WindowSystemGLFW::s_GLFWKeyCodeMap;
	std::unordered_map<int, EButtonCode> WindowSystemGLFW::s_GLFWButtonCodeMap;
	std::unordered_map<int, EInputType> WindowSystemGLFW::s_GLFWInputMap;


	WindowSystemGLFW::~WindowSystemGLFW()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}
	void WindowSystemGLFW::Initialize(const WindowInitInfo& initInfo)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, initInfo.resizeable);
		m_Window = glfwCreateWindow(initInfo.width, initInfo.height, initInfo.title, nullptr, nullptr);
		glfwSetWindowUserPointer(m_Window, (void*)this);
		if(s_GLFWButtonCodeMap.empty()) {
			InitKeyButtonCodeMap();
		}
		InitEvents();
	}
	void WindowSystemGLFW::PollEvents()
	{
		glfwPollEvents();
	}
	bool WindowSystemGLFW::ShouldClose()
	{
		return glfwWindowShouldClose(m_Window);
	}
	void WindowSystemGLFW::Close()
	{
		glfwSetWindowShouldClose(m_Window, 1);
	}
	void WindowSystemGLFW::SetTitle(const char* title)
	{
		glfwSetWindowTitle(m_Window, title);
	}
	void WindowSystemGLFW::GetWindowSize(int* w, int* h)
	{
		glfwGetWindowSize(m_Window, w, h);
	}

	void WindowSystemGLFW::SetFocusMode(bool focusMode)
	{
		m_FocusMode = focusMode;
		glfwSetInputMode(m_Window, GLFW_CURSOR, m_FocusMode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	bool WindowSystemGLFW::GetFocusMode()
	{
		return m_FocusMode;
	}

	void* WindowSystemGLFW::GetWindowHandle()
	{
		return (void*)m_Window;
	}

	void WindowSystemGLFW::GetWindowContentScale(float* x, float* y)
	{
		glfwGetWindowContentScale(m_Window, x, y);
	}

	void WindowSystemGLFW::SetWindowIcon(int count, const WindowIcon* icons)
	{
		const GLFWimage* images = reinterpret_cast<const GLFWimage*>(icons);
		glfwSetWindowIcon(m_Window, count, images);
	}

	void WindowSystemGLFW::InitKeyButtonCodeMap()
	{
		s_GLFWKeyCodeMap[GLFW_KEY_SPACE] = EKeyCode::SPACE;
		s_GLFWKeyCodeMap[GLFW_KEY_APOSTROPHE] = EKeyCode::APOSTROPHE; /* ' */
		s_GLFWKeyCodeMap[GLFW_KEY_COMMA] = EKeyCode::COMMA; /* ; */
		s_GLFWKeyCodeMap[GLFW_KEY_MINUS] = EKeyCode::MINUS; /* - */
		s_GLFWKeyCodeMap[GLFW_KEY_PERIOD] = EKeyCode::PERIOD; /* . */
		s_GLFWKeyCodeMap[GLFW_KEY_SLASH] = EKeyCode::SLASH; /* / */
		s_GLFWKeyCodeMap[GLFW_KEY_0] = EKeyCode::A0;
		s_GLFWKeyCodeMap[GLFW_KEY_1] = EKeyCode::A1;
		s_GLFWKeyCodeMap[GLFW_KEY_2] = EKeyCode::A2;
		s_GLFWKeyCodeMap[GLFW_KEY_3] = EKeyCode::A3;
		s_GLFWKeyCodeMap[GLFW_KEY_4] = EKeyCode::A4;
		s_GLFWKeyCodeMap[GLFW_KEY_5] = EKeyCode::A5;
		s_GLFWKeyCodeMap[GLFW_KEY_6] = EKeyCode::A6;
		s_GLFWKeyCodeMap[GLFW_KEY_7] = EKeyCode::A7;
		s_GLFWKeyCodeMap[GLFW_KEY_8] = EKeyCode::A8;
		s_GLFWKeyCodeMap[GLFW_KEY_9] = EKeyCode::A9;
		s_GLFWKeyCodeMap[GLFW_KEY_SEMICOLON] = EKeyCode::SEMICOLON; /* ; */
		s_GLFWKeyCodeMap[GLFW_KEY_EQUAL] = EKeyCode::EQUAL; /* = */
		s_GLFWKeyCodeMap[GLFW_KEY_A] = EKeyCode::A;
		s_GLFWKeyCodeMap[GLFW_KEY_B] = EKeyCode::B;
		s_GLFWKeyCodeMap[GLFW_KEY_C] = EKeyCode::C;
		s_GLFWKeyCodeMap[GLFW_KEY_D] = EKeyCode::D;
		s_GLFWKeyCodeMap[GLFW_KEY_E] = EKeyCode::E;
		s_GLFWKeyCodeMap[GLFW_KEY_F] = EKeyCode::F;
		s_GLFWKeyCodeMap[GLFW_KEY_G] = EKeyCode::G;
		s_GLFWKeyCodeMap[GLFW_KEY_H] = EKeyCode::H;
		s_GLFWKeyCodeMap[GLFW_KEY_I] = EKeyCode::I;
		s_GLFWKeyCodeMap[GLFW_KEY_J] = EKeyCode::J;
		s_GLFWKeyCodeMap[GLFW_KEY_K] = EKeyCode::K;
		s_GLFWKeyCodeMap[GLFW_KEY_L] = EKeyCode::L;
		s_GLFWKeyCodeMap[GLFW_KEY_M] = EKeyCode::M;
		s_GLFWKeyCodeMap[GLFW_KEY_N] = EKeyCode::N;
		s_GLFWKeyCodeMap[GLFW_KEY_O] = EKeyCode::O;
		s_GLFWKeyCodeMap[GLFW_KEY_P] = EKeyCode::P;
		s_GLFWKeyCodeMap[GLFW_KEY_Q] = EKeyCode::Q;
		s_GLFWKeyCodeMap[GLFW_KEY_R] = EKeyCode::R;
		s_GLFWKeyCodeMap[GLFW_KEY_S] = EKeyCode::S;
		s_GLFWKeyCodeMap[GLFW_KEY_T] = EKeyCode::T;
		s_GLFWKeyCodeMap[GLFW_KEY_U] = EKeyCode::U;
		s_GLFWKeyCodeMap[GLFW_KEY_V] = EKeyCode::V;
		s_GLFWKeyCodeMap[GLFW_KEY_W] = EKeyCode::W;
		s_GLFWKeyCodeMap[GLFW_KEY_X] = EKeyCode::X;
		s_GLFWKeyCodeMap[GLFW_KEY_Y] = EKeyCode::Y;
		s_GLFWKeyCodeMap[GLFW_KEY_Z] = EKeyCode::Z;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT_BRACKET] = EKeyCode::LEFT_BRACKET; /* [ */
		s_GLFWKeyCodeMap[GLFW_KEY_BACKSLASH] = EKeyCode::BACKSLASH; /* \ */
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT_BRACKET] = EKeyCode::RIGHT_BRACKET; /* ] */
		s_GLFWKeyCodeMap[GLFW_KEY_GRAVE_ACCENT] = EKeyCode::GRAVE_ACCENT; /* ` */
		s_GLFWKeyCodeMap[GLFW_KEY_WORLD_1] = EKeyCode::WORLD_1; /* non-US #1 */
		s_GLFWKeyCodeMap[GLFW_KEY_WORLD_2] = EKeyCode::WORLD_2; /* non-US #2 */
		s_GLFWKeyCodeMap[GLFW_KEY_ESCAPE] = EKeyCode::ESCAPE;
		s_GLFWKeyCodeMap[GLFW_KEY_ENTER] = EKeyCode::ENTER;
		s_GLFWKeyCodeMap[GLFW_KEY_TAB] = EKeyCode::TAB;
		s_GLFWKeyCodeMap[GLFW_KEY_BACKSPACE] = EKeyCode::BACKSPACE;
		s_GLFWKeyCodeMap[GLFW_KEY_INSERT] = EKeyCode::INSERT;
		s_GLFWKeyCodeMap[GLFW_KEY_DELETE] = EKeyCode::DELETE;
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT] = EKeyCode::RIGHT;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT] = EKeyCode::LEFT;
		s_GLFWKeyCodeMap[GLFW_KEY_DOWN] = EKeyCode::DOWN;
		s_GLFWKeyCodeMap[GLFW_KEY_UP] = EKeyCode::UP;
		s_GLFWKeyCodeMap[GLFW_KEY_PAGE_UP] = EKeyCode::PAGE_UP;
		s_GLFWKeyCodeMap[GLFW_KEY_PAGE_DOWN] = EKeyCode::PAGE_DOWN;
		s_GLFWKeyCodeMap[GLFW_KEY_HOME] = EKeyCode::HOME;
		s_GLFWKeyCodeMap[GLFW_KEY_END] = EKeyCode::END;
		s_GLFWKeyCodeMap[GLFW_KEY_CAPS_LOCK] = EKeyCode::CAPS_LOCK;
		s_GLFWKeyCodeMap[GLFW_KEY_SCROLL_LOCK] = EKeyCode::SCROLL_LOCK;
		s_GLFWKeyCodeMap[GLFW_KEY_NUM_LOCK] = EKeyCode::NUM_LOCK;
		s_GLFWKeyCodeMap[GLFW_KEY_PRINT_SCREEN] = EKeyCode::PRINT_SCREEN;
		s_GLFWKeyCodeMap[GLFW_KEY_PAUSE] = EKeyCode::PAUSE;
		s_GLFWKeyCodeMap[GLFW_KEY_F1] = EKeyCode::F1;
		s_GLFWKeyCodeMap[GLFW_KEY_F2] = EKeyCode::F2;
		s_GLFWKeyCodeMap[GLFW_KEY_F3] = EKeyCode::F3;
		s_GLFWKeyCodeMap[GLFW_KEY_F4] = EKeyCode::F4;
		s_GLFWKeyCodeMap[GLFW_KEY_F5] = EKeyCode::F5;
		s_GLFWKeyCodeMap[GLFW_KEY_F6] = EKeyCode::F6;
		s_GLFWKeyCodeMap[GLFW_KEY_F7] = EKeyCode::F7;
		s_GLFWKeyCodeMap[GLFW_KEY_F8] = EKeyCode::F8;
		s_GLFWKeyCodeMap[GLFW_KEY_F9] = EKeyCode::F9;
		s_GLFWKeyCodeMap[GLFW_KEY_F10] = EKeyCode::F10;
		s_GLFWKeyCodeMap[GLFW_KEY_F11] = EKeyCode::F11;
		s_GLFWKeyCodeMap[GLFW_KEY_F12] = EKeyCode::F12;
		s_GLFWKeyCodeMap[GLFW_KEY_F13] = EKeyCode::F13;
		s_GLFWKeyCodeMap[GLFW_KEY_F14] = EKeyCode::F14;
		s_GLFWKeyCodeMap[GLFW_KEY_F15] = EKeyCode::F15;
		s_GLFWKeyCodeMap[GLFW_KEY_F16] = EKeyCode::F16;
		s_GLFWKeyCodeMap[GLFW_KEY_F17] = EKeyCode::F17;
		s_GLFWKeyCodeMap[GLFW_KEY_F18] = EKeyCode::F18;
		s_GLFWKeyCodeMap[GLFW_KEY_F19] = EKeyCode::F19;
		s_GLFWKeyCodeMap[GLFW_KEY_F20] = EKeyCode::F20;
		s_GLFWKeyCodeMap[GLFW_KEY_F21] = EKeyCode::F21;
		s_GLFWKeyCodeMap[GLFW_KEY_F22] = EKeyCode::F22;
		s_GLFWKeyCodeMap[GLFW_KEY_F23] = EKeyCode::F23;
		s_GLFWKeyCodeMap[GLFW_KEY_F24] = EKeyCode::F24;
		s_GLFWKeyCodeMap[GLFW_KEY_F25] = EKeyCode::F25;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_0] = EKeyCode::KP_0;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_1] = EKeyCode::KP_1;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_2] = EKeyCode::KP_2;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_3] = EKeyCode::KP_3;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_4] = EKeyCode::KP_4;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_5] = EKeyCode::KP_5;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_6] = EKeyCode::KP_6;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_7] = EKeyCode::KP_7;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_8] = EKeyCode::KP_8;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_9] = EKeyCode::KP_9;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_DECIMAL] = EKeyCode::KP_DECIMAL;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_DIVIDE] = EKeyCode::KP_DIVIDE;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_MULTIPLY] = EKeyCode::KP_MULTIPLY;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_SUBTRACT] = EKeyCode::KP_SUBTRACT;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_ADD] = EKeyCode::KP_ADD;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_ENTER] = EKeyCode::KP_ENTER;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_EQUAL] = EKeyCode::KP_EQUAL;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT_SHIFT] = EKeyCode::LEFT_SHIFT;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT_CONTROL] = EKeyCode::LEFT_CONTROL;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT_ALT] = EKeyCode::LEFT_ALT;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT_SUPER] = EKeyCode::LEFT_SUPER;
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT_SHIFT] = EKeyCode::RIGHT_SHIFT;
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT_CONTROL] = EKeyCode::RIGHT_CONTROL;
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT_ALT] = EKeyCode::RIGHT_ALT;
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT_SUPER] = EKeyCode::RIGHT_SUPER;
		s_GLFWKeyCodeMap[GLFW_KEY_MENU] = EKeyCode::MENU;

		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_1] = EButtonCode::BTN_1;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_2] = EButtonCode::BTN_2;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_3] = EButtonCode::BTN_3;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_4] = EButtonCode::BTN_4;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_5] = EButtonCode::BTN_5;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_6] = EButtonCode::BTN_6;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_7] = EButtonCode::BTN_7;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_8] = EButtonCode::BTN_8;

		s_GLFWInputMap[GLFW_PRESS] = EInputType::PRESS;
		s_GLFWInputMap[GLFW_RELEASE] = EInputType::RELEASE;
		s_GLFWInputMap[GLFW_REPEAT] = EInputType::REPEAT;
	}

	void WindowSystemGLFW::InitEvents()
	{
		glfwSetKeyCallback(m_Window, OnKey);
		glfwSetMouseButtonCallback(m_Window, OnMouseButton);
		glfwSetCursorPosCallback(m_Window, OnCursorPos);
		glfwSetCursorEnterCallback(m_Window, OnCusorEnterFunc);
		glfwSetScrollCallback(m_Window, OnScroll);
		glfwSetWindowSizeCallback(m_Window, OnWindowSize);
	}


	// ×¢²áÊÂ¼þ

	void WindowSystemGLFW::RegisterOnKeyFunc(OnKeyFunc func)
	{
		m_OnKeyFunc.push_back(func);
	}

	void WindowSystemGLFW::RegisterOnMouseButtonFunc(OnMouseButtonFunc func)
	{
		m_OnMouseButtonFunc.push_back(func);
	}

	void WindowSystemGLFW::RegisterOnCursorPosFunc(OnCursorPosFunc func)
	{
		m_OnCursorPosFunc.push_back(func);
	}

	void WindowSystemGLFW::RegisterOnCursorEnterFunc(OnCursorEnterFunc func)
	{
		m_OnCursorEnterFunc.push_back(func);
	}

	void WindowSystemGLFW::RegisterOnScrollFunc(OnScrollFunc func)
	{
		m_OnScrollFunc.push_back(func);
	}

	void WindowSystemGLFW::RegisterOnWindowSizeFunc(OnWindowSizeFunc func)
	{
		m_OnWindowSizeFunc.push_back(func);
	}


	// ´¥·¢
	void WindowSystemGLFW::OnKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnKeyFunc) {
			func(s_GLFWKeyCodeMap[key], s_GLFWInputMap[action]);
		}
	}
	void WindowSystemGLFW::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
	{
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnMouseButtonFunc) {
			func(s_GLFWButtonCodeMap[button], s_GLFWInputMap[action]);
		}
	}
	void WindowSystemGLFW::OnCursorPos(GLFWwindow* window, double x, double y)
	{
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnCursorPosFunc) {
			func(x, y);
		}
	}
	void WindowSystemGLFW::OnCusorEnterFunc(GLFWwindow* window, int entered)
	{
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnCursorEnterFunc) {
			func(static_cast<bool>(entered));
		}
	}
	void WindowSystemGLFW::OnScroll(GLFWwindow* window, double x, double y)
	{
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnScrollFunc) {
			func(x, y);
		}
	}
	void WindowSystemGLFW::OnWindowSize(GLFWwindow* window, int width, int height)
	{
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnWindowSizeFunc) {
			func(width, height);
		}
	}
}
