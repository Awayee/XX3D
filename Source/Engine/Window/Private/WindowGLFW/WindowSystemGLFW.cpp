#include "WindowSystemGLFW.h"
#include "Engine/Public/Engine.h"

namespace Engine {

	TUnorderedMap<int, EKey> WindowSystemGLFW::s_GLFWKeyCodeMap;
	TUnorderedMap<int, EBtn> WindowSystemGLFW::s_GLFWButtonCodeMap;
	TUnorderedMap<int, EInput> WindowSystemGLFW::s_GLFWInputMap;

	WindowSystemGLFW::WindowSystemGLFW(const WindowInitInfo& initInfo) {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, initInfo.resizeable);
		m_Window = glfwCreateWindow(initInfo.width, initInfo.height, initInfo.title, nullptr, nullptr);
		glfwSetWindowUserPointer(m_Window, (void*)this);
		if (s_GLFWButtonCodeMap.empty()) {
			InitKeyButtonCodeMap();
		}
		m_Size.w = static_cast<uint32>(initInfo.width);
		m_Size.h = static_cast<uint32>(initInfo.height);
		InitEvents();
	}

	WindowSystemGLFW::~WindowSystemGLFW() {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void WindowSystemGLFW::Update(){
		glfwPollEvents();
		if(glfwWindowShouldClose(m_Window)) {
			Engine::XXEngine::ShutDown();
		}
	}

	void WindowSystemGLFW::Close(){
		glfwSetWindowShouldClose(m_Window, 1);
	}

	void WindowSystemGLFW::SetTitle(const char* title){
		glfwSetWindowTitle(m_Window, title);
	}

	void WindowSystemGLFW::SetFocusMode(bool focusMode){
		m_FocusMode = focusMode;
		glfwSetInputMode(m_Window, GLFW_CURSOR, m_FocusMode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	void WindowSystemGLFW::SetWindowIcon(int count, const WindowIcon* icons) {
		const GLFWimage* images = reinterpret_cast<const GLFWimage*>(icons);
		glfwSetWindowIcon(m_Window, count, images);
	}

	bool WindowSystemGLFW::GetFocusMode(){
		return m_FocusMode;
	}

	void* WindowSystemGLFW::GetWindowHandle(){
		return (void*)m_Window;
	}

	USize2D WindowSystemGLFW::GetWindowSize() {
		return m_Size;
	}

	FSize2D WindowSystemGLFW::GetWindowContentScale() {
		FSize2D scale;
		glfwGetWindowContentScale(m_Window, &scale.w, &scale.h);
		return scale;
	}

	FOffset2D WindowSystemGLFW::GetCursorPos() {
		double x, y;
		glfwGetCursorPos(m_Window, &x, &y);
		return { (float)x, (float)y };
	}

	void WindowSystemGLFW::InitKeyButtonCodeMap(){
		s_GLFWKeyCodeMap[GLFW_KEY_SPACE] = EKey::SPACE;
		s_GLFWKeyCodeMap[GLFW_KEY_APOSTROPHE] = EKey::APOSTROPHE; /* ' */
		s_GLFWKeyCodeMap[GLFW_KEY_COMMA] = EKey::COMMA; /* ; */
		s_GLFWKeyCodeMap[GLFW_KEY_MINUS] = EKey::MINUS; /* - */
		s_GLFWKeyCodeMap[GLFW_KEY_PERIOD] = EKey::PERIOD; /* . */
		s_GLFWKeyCodeMap[GLFW_KEY_SLASH] = EKey::SLASH; /* / */
		s_GLFWKeyCodeMap[GLFW_KEY_0] = EKey::A0;
		s_GLFWKeyCodeMap[GLFW_KEY_1] = EKey::A1;
		s_GLFWKeyCodeMap[GLFW_KEY_2] = EKey::A2;
		s_GLFWKeyCodeMap[GLFW_KEY_3] = EKey::A3;
		s_GLFWKeyCodeMap[GLFW_KEY_4] = EKey::A4;
		s_GLFWKeyCodeMap[GLFW_KEY_5] = EKey::A5;
		s_GLFWKeyCodeMap[GLFW_KEY_6] = EKey::A6;
		s_GLFWKeyCodeMap[GLFW_KEY_7] = EKey::A7;
		s_GLFWKeyCodeMap[GLFW_KEY_8] = EKey::A8;
		s_GLFWKeyCodeMap[GLFW_KEY_9] = EKey::A9;
		s_GLFWKeyCodeMap[GLFW_KEY_SEMICOLON] = EKey::SEMICOLON; /* ; */
		s_GLFWKeyCodeMap[GLFW_KEY_EQUAL] = EKey::EQUAL; /* = */
		s_GLFWKeyCodeMap[GLFW_KEY_A] = EKey::A;
		s_GLFWKeyCodeMap[GLFW_KEY_B] = EKey::B;
		s_GLFWKeyCodeMap[GLFW_KEY_C] = EKey::C;
		s_GLFWKeyCodeMap[GLFW_KEY_D] = EKey::D;
		s_GLFWKeyCodeMap[GLFW_KEY_E] = EKey::E;
		s_GLFWKeyCodeMap[GLFW_KEY_F] = EKey::F;
		s_GLFWKeyCodeMap[GLFW_KEY_G] = EKey::G;
		s_GLFWKeyCodeMap[GLFW_KEY_H] = EKey::H;
		s_GLFWKeyCodeMap[GLFW_KEY_I] = EKey::I;
		s_GLFWKeyCodeMap[GLFW_KEY_J] = EKey::J;
		s_GLFWKeyCodeMap[GLFW_KEY_K] = EKey::K;
		s_GLFWKeyCodeMap[GLFW_KEY_L] = EKey::L;
		s_GLFWKeyCodeMap[GLFW_KEY_M] = EKey::M;
		s_GLFWKeyCodeMap[GLFW_KEY_N] = EKey::N;
		s_GLFWKeyCodeMap[GLFW_KEY_O] = EKey::O;
		s_GLFWKeyCodeMap[GLFW_KEY_P] = EKey::P;
		s_GLFWKeyCodeMap[GLFW_KEY_Q] = EKey::Q;
		s_GLFWKeyCodeMap[GLFW_KEY_R] = EKey::R;
		s_GLFWKeyCodeMap[GLFW_KEY_S] = EKey::S;
		s_GLFWKeyCodeMap[GLFW_KEY_T] = EKey::T;
		s_GLFWKeyCodeMap[GLFW_KEY_U] = EKey::U;
		s_GLFWKeyCodeMap[GLFW_KEY_V] = EKey::V;
		s_GLFWKeyCodeMap[GLFW_KEY_W] = EKey::W;
		s_GLFWKeyCodeMap[GLFW_KEY_X] = EKey::X;
		s_GLFWKeyCodeMap[GLFW_KEY_Y] = EKey::Y;
		s_GLFWKeyCodeMap[GLFW_KEY_Z] = EKey::Z;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT_BRACKET] = EKey::LEFT_BRACKET; /* [ */
		s_GLFWKeyCodeMap[GLFW_KEY_BACKSLASH] = EKey::BACKSLASH; /* \ */
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT_BRACKET] = EKey::RIGHT_BRACKET; /* ] */
		s_GLFWKeyCodeMap[GLFW_KEY_GRAVE_ACCENT] = EKey::GRAVE_ACCENT; /* ` */
		s_GLFWKeyCodeMap[GLFW_KEY_WORLD_1] = EKey::WORLD_1; /* non-US #1 */
		s_GLFWKeyCodeMap[GLFW_KEY_WORLD_2] = EKey::WORLD_2; /* non-US #2 */
		s_GLFWKeyCodeMap[GLFW_KEY_ESCAPE] = EKey::ESCAPE;
		s_GLFWKeyCodeMap[GLFW_KEY_ENTER] = EKey::ENTER;
		s_GLFWKeyCodeMap[GLFW_KEY_TAB] = EKey::TAB;
		s_GLFWKeyCodeMap[GLFW_KEY_BACKSPACE] = EKey::BACKSPACE;
		s_GLFWKeyCodeMap[GLFW_KEY_INSERT] = EKey::INSERT;
		s_GLFWKeyCodeMap[GLFW_KEY_DELETE] = EKey::DEL;
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT] = EKey::RIGHT;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT] = EKey::LEFT;
		s_GLFWKeyCodeMap[GLFW_KEY_DOWN] = EKey::DOWN;
		s_GLFWKeyCodeMap[GLFW_KEY_UP] = EKey::UP;
		s_GLFWKeyCodeMap[GLFW_KEY_PAGE_UP] = EKey::PAGE_UP;
		s_GLFWKeyCodeMap[GLFW_KEY_PAGE_DOWN] = EKey::PAGE_DOWN;
		s_GLFWKeyCodeMap[GLFW_KEY_HOME] = EKey::HOME;
		s_GLFWKeyCodeMap[GLFW_KEY_END] = EKey::END;
		s_GLFWKeyCodeMap[GLFW_KEY_CAPS_LOCK] = EKey::CAPS_LOCK;
		s_GLFWKeyCodeMap[GLFW_KEY_SCROLL_LOCK] = EKey::SCROLL_LOCK;
		s_GLFWKeyCodeMap[GLFW_KEY_NUM_LOCK] = EKey::NUM_LOCK;
		s_GLFWKeyCodeMap[GLFW_KEY_PRINT_SCREEN] = EKey::PRINT_SCREEN;
		s_GLFWKeyCodeMap[GLFW_KEY_PAUSE] = EKey::PAUSE;
		s_GLFWKeyCodeMap[GLFW_KEY_F1] = EKey::F1;
		s_GLFWKeyCodeMap[GLFW_KEY_F2] = EKey::F2;
		s_GLFWKeyCodeMap[GLFW_KEY_F3] = EKey::F3;
		s_GLFWKeyCodeMap[GLFW_KEY_F4] = EKey::F4;
		s_GLFWKeyCodeMap[GLFW_KEY_F5] = EKey::F5;
		s_GLFWKeyCodeMap[GLFW_KEY_F6] = EKey::F6;
		s_GLFWKeyCodeMap[GLFW_KEY_F7] = EKey::F7;
		s_GLFWKeyCodeMap[GLFW_KEY_F8] = EKey::F8;
		s_GLFWKeyCodeMap[GLFW_KEY_F9] = EKey::F9;
		s_GLFWKeyCodeMap[GLFW_KEY_F10] = EKey::F10;
		s_GLFWKeyCodeMap[GLFW_KEY_F11] = EKey::F11;
		s_GLFWKeyCodeMap[GLFW_KEY_F12] = EKey::F12;
		s_GLFWKeyCodeMap[GLFW_KEY_F13] = EKey::F13;
		s_GLFWKeyCodeMap[GLFW_KEY_F14] = EKey::F14;
		s_GLFWKeyCodeMap[GLFW_KEY_F15] = EKey::F15;
		s_GLFWKeyCodeMap[GLFW_KEY_F16] = EKey::F16;
		s_GLFWKeyCodeMap[GLFW_KEY_F17] = EKey::F17;
		s_GLFWKeyCodeMap[GLFW_KEY_F18] = EKey::F18;
		s_GLFWKeyCodeMap[GLFW_KEY_F19] = EKey::F19;
		s_GLFWKeyCodeMap[GLFW_KEY_F20] = EKey::F20;
		s_GLFWKeyCodeMap[GLFW_KEY_F21] = EKey::F21;
		s_GLFWKeyCodeMap[GLFW_KEY_F22] = EKey::F22;
		s_GLFWKeyCodeMap[GLFW_KEY_F23] = EKey::F23;
		s_GLFWKeyCodeMap[GLFW_KEY_F24] = EKey::F24;
		s_GLFWKeyCodeMap[GLFW_KEY_F25] = EKey::F25;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_0] = EKey::KP_0;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_1] = EKey::KP_1;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_2] = EKey::KP_2;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_3] = EKey::KP_3;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_4] = EKey::KP_4;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_5] = EKey::KP_5;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_6] = EKey::KP_6;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_7] = EKey::KP_7;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_8] = EKey::KP_8;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_9] = EKey::KP_9;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_DECIMAL] = EKey::KP_DECIMAL;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_DIVIDE] = EKey::KP_DIVIDE;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_MULTIPLY] = EKey::KP_MULTIPLY;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_SUBTRACT] = EKey::KP_SUBTRACT;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_ADD] = EKey::KP_ADD;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_ENTER] = EKey::KP_ENTER;
		s_GLFWKeyCodeMap[GLFW_KEY_KP_EQUAL] = EKey::KP_EQUAL;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT_SHIFT] = EKey::LEFT_SHIFT;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT_CONTROL] = EKey::LEFT_CONTROL;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT_ALT] = EKey::LEFT_ALT;
		s_GLFWKeyCodeMap[GLFW_KEY_LEFT_SUPER] = EKey::LEFT_SUPER;
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT_SHIFT] = EKey::RIGHT_SHIFT;
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT_CONTROL] = EKey::RIGHT_CONTROL;
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT_ALT] = EKey::RIGHT_ALT;
		s_GLFWKeyCodeMap[GLFW_KEY_RIGHT_SUPER] = EKey::RIGHT_SUPER;
		s_GLFWKeyCodeMap[GLFW_KEY_MENU] = EKey::MENU;

		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_1] = EBtn::LEFT;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_2] = EBtn::RIGHT;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_3] = EBtn::MIDDLE;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_4] = EBtn::BTN_4;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_5] = EBtn::BTN_5;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_6] = EBtn::BTN_6;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_7] = EBtn::BTN_7;
		s_GLFWButtonCodeMap[GLFW_MOUSE_BUTTON_8] = EBtn::BTN_8;

		s_GLFWInputMap[GLFW_PRESS] = EInput::PRESS;
		s_GLFWInputMap[GLFW_RELEASE] = EInput::RELEASE;
		s_GLFWInputMap[GLFW_REPEAT] = EInput::REPEAT;
	}

	void WindowSystemGLFW::InitEvents(){
		glfwSetKeyCallback(m_Window, OnKey);
		glfwSetMouseButtonCallback(m_Window, OnMouseButton);
		glfwSetCursorPosCallback(m_Window, OnCursorPos);
		glfwSetCursorEnterCallback(m_Window, OnCursorEnter);
		glfwSetScrollCallback(m_Window, OnScroll);
		glfwSetWindowSizeCallback(m_Window, OnWindowSize);
		glfwSetWindowFocusCallback(m_Window, OnWindowFocus);
	}

	void WindowSystemGLFW::RegisterOnKeyFunc(OnKeyFunc&& func){
		m_OnKeyFunc.PushBack(MoveTemp(func));
	}

	void WindowSystemGLFW::RegisterOnMouseButtonFunc(OnMouseButtonFunc&& func){
		m_OnMouseButtonFunc.PushBack(MoveTemp(func));
	}

	void WindowSystemGLFW::RegisterOnCursorPosFunc(OnCursorPosFunc&& func){
		m_OnCursorPosFunc.PushBack(MoveTemp(func));
	}

	void WindowSystemGLFW::RegisterOnCursorEnterFunc(OnCursorEnterFunc&& func){
		m_OnCursorEnterFunc.PushBack(MoveTemp(func));
	}

	void WindowSystemGLFW::RegisterOnScrollFunc(OnScrollFunc&& func){
		m_OnScrollFunc.PushBack(MoveTemp(func));
	}

	void WindowSystemGLFW::RegisterOnWindowSizeFunc(OnWindowSizeFunc&& func){
		m_OnWindowSizeFunc.PushBack(MoveTemp(func));
	}


	// ´¥·¢
	void WindowSystemGLFW::OnKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnKeyFunc) {
			func(s_GLFWKeyCodeMap[key], s_GLFWInputMap[action]);
		}
	}
	void WindowSystemGLFW::OnMouseButton(GLFWwindow* window, int button, int action, int mods){
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnMouseButtonFunc) {
			func(s_GLFWButtonCodeMap[button], s_GLFWInputMap[action]);
		}
	}
	void WindowSystemGLFW::OnCursorPos(GLFWwindow* window, double x, double y){
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnCursorPosFunc) {
			func(x, y);
		}
	}
	void WindowSystemGLFW::OnCursorEnter(GLFWwindow* window, int entered){
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnCursorEnterFunc) {
			func(static_cast<bool>(entered));
		}
	}
	void WindowSystemGLFW::OnScroll(GLFWwindow* window, double x, double y){
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnScrollFunc) {
			func(x, y);
		}
	}
	void WindowSystemGLFW::OnWindowSize(GLFWwindow* window, int width, int height){
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		sys->m_Size.w = width;
		sys->m_Size.h = height;
		for(auto& func: sys->m_OnWindowSizeFunc) {
			func(width, height);
		}
	}

	void WindowSystemGLFW::OnWindowFocus(GLFWwindow* window, int focus) {
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnWindowFocusFunc) {
			func(focus);
		}
	}

}
