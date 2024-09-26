#include "WindowSystemGLFW.h"
#include "Engine/Public/Engine.h"

namespace Engine {

	WindowSystemGLFW::WindowSystemGLFW(const WindowInitInfo& initInfo) {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, initInfo.Resizeable);
		m_Window = glfwCreateWindow(initInfo.Width, initInfo.Height, initInfo.Title, nullptr, nullptr);
		glfwSetWindowUserPointer(m_Window, (void*)this);
		InitKeyButtonCodeMap();
		m_Size.w = static_cast<uint32>(initInfo.Width);
		m_Size.h = static_cast<uint32>(initInfo.Height);
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

	bool WindowSystemGLFW::IsKeyDown(EKey key) {
		return GLFW_PRESS == glfwGetKey(m_Window, m_KeyMap.GetNum(key));
	}

	bool WindowSystemGLFW::IsMouseDown(EBtn btn) {
		return GLFW_PRESS == glfwGetMouseButton(m_Window, m_MouseButtonMap.GetNum(btn));
	}

	void WindowSystemGLFW::InitKeyButtonCodeMap(){
#define PARSE_KEY_CODE(glfwKey, key) m_KeyMap.AddPair(glfwKey, key)
		PARSE_KEY_CODE(GLFW_KEY_SPACE, EKey::Space);
		PARSE_KEY_CODE(GLFW_KEY_APOSTROPHE, EKey::Apostrophe); /* ' */
		PARSE_KEY_CODE(GLFW_KEY_COMMA, EKey::Comma); /* ; */
		PARSE_KEY_CODE(GLFW_KEY_MINUS, EKey::Minus); /* - */
		PARSE_KEY_CODE(GLFW_KEY_PERIOD, EKey::Period); /* . */
		PARSE_KEY_CODE(GLFW_KEY_SLASH, EKey::Slash); /* / */
		PARSE_KEY_CODE(GLFW_KEY_0, EKey::A0);
		PARSE_KEY_CODE(GLFW_KEY_1, EKey::A1);
		PARSE_KEY_CODE(GLFW_KEY_2, EKey::A2);
		PARSE_KEY_CODE(GLFW_KEY_3, EKey::A3);
		PARSE_KEY_CODE(GLFW_KEY_4, EKey::A4);
		PARSE_KEY_CODE(GLFW_KEY_5, EKey::A5);
		PARSE_KEY_CODE(GLFW_KEY_6, EKey::A6);
		PARSE_KEY_CODE(GLFW_KEY_7, EKey::A7);
		PARSE_KEY_CODE(GLFW_KEY_8, EKey::A8);
		PARSE_KEY_CODE(GLFW_KEY_9, EKey::A9);
		PARSE_KEY_CODE(GLFW_KEY_SEMICOLON, EKey::Semicolon); /* ; */
		PARSE_KEY_CODE(GLFW_KEY_EQUAL, EKey::Equal); /* = */
		PARSE_KEY_CODE(GLFW_KEY_A, EKey::A);
		PARSE_KEY_CODE(GLFW_KEY_B, EKey::B);
		PARSE_KEY_CODE(GLFW_KEY_C, EKey::C);
		PARSE_KEY_CODE(GLFW_KEY_D, EKey::D);
		PARSE_KEY_CODE(GLFW_KEY_E, EKey::E);
		PARSE_KEY_CODE(GLFW_KEY_F, EKey::F);
		PARSE_KEY_CODE(GLFW_KEY_G, EKey::G);
		PARSE_KEY_CODE(GLFW_KEY_H, EKey::H);
		PARSE_KEY_CODE(GLFW_KEY_I, EKey::I);
		PARSE_KEY_CODE(GLFW_KEY_J, EKey::J);
		PARSE_KEY_CODE(GLFW_KEY_K, EKey::K);
		PARSE_KEY_CODE(GLFW_KEY_L, EKey::L);
		PARSE_KEY_CODE(GLFW_KEY_M, EKey::M);
		PARSE_KEY_CODE(GLFW_KEY_N, EKey::N);
		PARSE_KEY_CODE(GLFW_KEY_O, EKey::O);
		PARSE_KEY_CODE(GLFW_KEY_P, EKey::P);
		PARSE_KEY_CODE(GLFW_KEY_Q, EKey::Q);
		PARSE_KEY_CODE(GLFW_KEY_R, EKey::R);
		PARSE_KEY_CODE(GLFW_KEY_S, EKey::S);
		PARSE_KEY_CODE(GLFW_KEY_T, EKey::T);
		PARSE_KEY_CODE(GLFW_KEY_U, EKey::U);
		PARSE_KEY_CODE(GLFW_KEY_V, EKey::V);
		PARSE_KEY_CODE(GLFW_KEY_W, EKey::W);
		PARSE_KEY_CODE(GLFW_KEY_X, EKey::X);
		PARSE_KEY_CODE(GLFW_KEY_Y, EKey::Y);
		PARSE_KEY_CODE(GLFW_KEY_Z, EKey::Z);
		PARSE_KEY_CODE(GLFW_KEY_LEFT_BRACKET, EKey::LeftBracket); /* [ */
		PARSE_KEY_CODE(GLFW_KEY_BACKSLASH, EKey::BackSlash); /* \ */
		PARSE_KEY_CODE(GLFW_KEY_RIGHT_BRACKET, EKey::RightBracket); /* ] */
		PARSE_KEY_CODE(GLFW_KEY_GRAVE_ACCENT, EKey::GraveAccent); /* ` */
		PARSE_KEY_CODE(GLFW_KEY_WORLD_1, EKey::World1); /* non-US #1 */
		PARSE_KEY_CODE(GLFW_KEY_WORLD_2, EKey::World2); /* non-US #2 */
		PARSE_KEY_CODE(GLFW_KEY_ESCAPE, EKey::Escape);
		PARSE_KEY_CODE(GLFW_KEY_ENTER, EKey::Enter);
		PARSE_KEY_CODE(GLFW_KEY_TAB, EKey::Tab);
		PARSE_KEY_CODE(GLFW_KEY_BACKSPACE, EKey::Backspace);
		PARSE_KEY_CODE(GLFW_KEY_INSERT, EKey::Insert);
		PARSE_KEY_CODE(GLFW_KEY_DELETE, EKey::Delete);
		PARSE_KEY_CODE(GLFW_KEY_RIGHT, EKey::Right);
		PARSE_KEY_CODE(GLFW_KEY_LEFT, EKey::Left);
		PARSE_KEY_CODE(GLFW_KEY_DOWN, EKey::Down);
		PARSE_KEY_CODE(GLFW_KEY_UP, EKey::Up);
		PARSE_KEY_CODE(GLFW_KEY_PAGE_UP, EKey::PageUp);
		PARSE_KEY_CODE(GLFW_KEY_PAGE_DOWN, EKey::PageDown);
		PARSE_KEY_CODE(GLFW_KEY_HOME, EKey::Home);
		PARSE_KEY_CODE(GLFW_KEY_END, EKey::End);
		PARSE_KEY_CODE(GLFW_KEY_CAPS_LOCK, EKey::CapsLock);
		PARSE_KEY_CODE(GLFW_KEY_SCROLL_LOCK, EKey::ScrollLock);
		PARSE_KEY_CODE(GLFW_KEY_NUM_LOCK, EKey::NumLock);
		PARSE_KEY_CODE(GLFW_KEY_PRINT_SCREEN, EKey::PrtSc);
		PARSE_KEY_CODE(GLFW_KEY_PAUSE, EKey::Pause);
		PARSE_KEY_CODE(GLFW_KEY_F1, EKey::F1);
		PARSE_KEY_CODE(GLFW_KEY_F2, EKey::F2);
		PARSE_KEY_CODE(GLFW_KEY_F3, EKey::F3);
		PARSE_KEY_CODE(GLFW_KEY_F4, EKey::F4);
		PARSE_KEY_CODE(GLFW_KEY_F5, EKey::F5);
		PARSE_KEY_CODE(GLFW_KEY_F6, EKey::F6);
		PARSE_KEY_CODE(GLFW_KEY_F7, EKey::F7);
		PARSE_KEY_CODE(GLFW_KEY_F8, EKey::F8);
		PARSE_KEY_CODE(GLFW_KEY_F9, EKey::F9);
		PARSE_KEY_CODE(GLFW_KEY_F10, EKey::F10);
		PARSE_KEY_CODE(GLFW_KEY_F11, EKey::F11);
		PARSE_KEY_CODE(GLFW_KEY_F12, EKey::F12);
		PARSE_KEY_CODE(GLFW_KEY_F13, EKey::F13);
		PARSE_KEY_CODE(GLFW_KEY_F14, EKey::F14);
		PARSE_KEY_CODE(GLFW_KEY_F15, EKey::F15);
		PARSE_KEY_CODE(GLFW_KEY_F16, EKey::F16);
		PARSE_KEY_CODE(GLFW_KEY_F17, EKey::F17);
		PARSE_KEY_CODE(GLFW_KEY_F18, EKey::F18);
		PARSE_KEY_CODE(GLFW_KEY_F19, EKey::F19);
		PARSE_KEY_CODE(GLFW_KEY_F20, EKey::F20);
		PARSE_KEY_CODE(GLFW_KEY_F21, EKey::F21);
		PARSE_KEY_CODE(GLFW_KEY_F22, EKey::F22);
		PARSE_KEY_CODE(GLFW_KEY_F23, EKey::F23);
		PARSE_KEY_CODE(GLFW_KEY_F24, EKey::F24);
		PARSE_KEY_CODE(GLFW_KEY_F25, EKey::F25);
		PARSE_KEY_CODE(GLFW_KEY_KP_0, EKey::KP0);
		PARSE_KEY_CODE(GLFW_KEY_KP_1, EKey::KP1);
		PARSE_KEY_CODE(GLFW_KEY_KP_2, EKey::KP2);
		PARSE_KEY_CODE(GLFW_KEY_KP_3, EKey::KP3);
		PARSE_KEY_CODE(GLFW_KEY_KP_4, EKey::KP4);
		PARSE_KEY_CODE(GLFW_KEY_KP_5, EKey::KP5);
		PARSE_KEY_CODE(GLFW_KEY_KP_6, EKey::KP6);
		PARSE_KEY_CODE(GLFW_KEY_KP_7, EKey::KP7);
		PARSE_KEY_CODE(GLFW_KEY_KP_8, EKey::KP8);
		PARSE_KEY_CODE(GLFW_KEY_KP_9, EKey::KP9);
		PARSE_KEY_CODE(GLFW_KEY_KP_DECIMAL, EKey::KPDecimal);
		PARSE_KEY_CODE(GLFW_KEY_KP_DIVIDE, EKey::KPDivide);
		PARSE_KEY_CODE(GLFW_KEY_KP_MULTIPLY, EKey::KPMultiply);
		PARSE_KEY_CODE(GLFW_KEY_KP_SUBTRACT, EKey::KPSubtract);
		PARSE_KEY_CODE(GLFW_KEY_KP_ADD, EKey::KPAdd);
		PARSE_KEY_CODE(GLFW_KEY_KP_ENTER, EKey::KPEnter);
		PARSE_KEY_CODE(GLFW_KEY_KP_EQUAL, EKey::KPEqual);
		PARSE_KEY_CODE(GLFW_KEY_LEFT_SHIFT, EKey::LeftShit);
		PARSE_KEY_CODE(GLFW_KEY_LEFT_CONTROL, EKey::LeftCtrl);
		PARSE_KEY_CODE(GLFW_KEY_LEFT_ALT, EKey::LeftAlt);
		PARSE_KEY_CODE(GLFW_KEY_LEFT_SUPER, EKey::LeftSuper);
		PARSE_KEY_CODE(GLFW_KEY_RIGHT_SHIFT, EKey::RightShift);
		PARSE_KEY_CODE(GLFW_KEY_RIGHT_CONTROL, EKey::RightCtrl);
		PARSE_KEY_CODE(GLFW_KEY_RIGHT_ALT, EKey::RightAlt);
		PARSE_KEY_CODE(GLFW_KEY_RIGHT_SUPER, EKey::RightSuper);
		PARSE_KEY_CODE(GLFW_KEY_MENU, EKey::Menu);
#undef PARSE_KEY_CODE

#define PARSE_BTN_CODE(glfwKey, key) m_MouseButtonMap.AddPair(glfwKey, key)
		PARSE_BTN_CODE(GLFW_MOUSE_BUTTON_1, EBtn::Left);
		PARSE_BTN_CODE(GLFW_MOUSE_BUTTON_2, EBtn::Right);
		PARSE_BTN_CODE(GLFW_MOUSE_BUTTON_3, EBtn::Middle);
		PARSE_BTN_CODE(GLFW_MOUSE_BUTTON_4, EBtn::Btn4);
		PARSE_BTN_CODE(GLFW_MOUSE_BUTTON_5, EBtn::Btn5);
		PARSE_BTN_CODE(GLFW_MOUSE_BUTTON_6, EBtn::Btn6);
		PARSE_BTN_CODE(GLFW_MOUSE_BUTTON_7, EBtn::Btn7);
		PARSE_BTN_CODE(GLFW_MOUSE_BUTTON_8, EBtn::Btn8);
#undef PARSE_BTN_CODE

#define PARSE_INPUT_CODE(glfwKey, key) m_InputMap.AddPair(glfwKey, key)
		PARSE_INPUT_CODE(GLFW_PRESS, EInput::Press);
		PARSE_INPUT_CODE(GLFW_RELEASE, EInput::Release);
		PARSE_INPUT_CODE(GLFW_REPEAT, EInput::Repeat);
#undef PARSE_INPUT_CODE
	}

	void WindowSystemGLFW::InitEvents(){
		glfwSetKeyCallback(m_Window, OnKey);
		glfwSetMouseButtonCallback(m_Window, OnMouseButton);
		glfwSetCursorPosCallback(m_Window, OnCursorPos);
		glfwSetScrollCallback(m_Window, OnScroll);
		glfwSetWindowSizeCallback(m_Window, OnWindowSize);
		glfwSetWindowFocusCallback(m_Window, OnWindowFocus);
		glfwSetDropCallback(m_Window, OnDrop);
	}


	// ´¥·¢
	void WindowSystemGLFW::OnKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnKeyFunc) {
			func(sys->m_KeyMap.GetEnum(key), sys->m_InputMap.GetEnum(action));
		}
	}
	void WindowSystemGLFW::OnMouseButton(GLFWwindow* window, int button, int action, int mods){
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnMouseButtonFunc) {
			func(sys->m_MouseButtonMap.GetEnum(button), sys->m_InputMap.GetEnum(action));
		}
	}
	void WindowSystemGLFW::OnCursorPos(GLFWwindow* window, double x, double y){
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnCursorPosFunc) {
			func((float)x, (float)y);
		}
	}
	void WindowSystemGLFW::OnScroll(GLFWwindow* window, double x, double y){
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for(auto& func: sys->m_OnScrollFunc) {
			func((float)x, (float)y);
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

	void WindowSystemGLFW::OnDrop(GLFWwindow* window, int pathNum, const char** paths) {
		WindowSystemGLFW* sys = reinterpret_cast<WindowSystemGLFW*>(glfwGetWindowUserPointer(window));
		for (auto& func : sys->m_OnDropFunc) {
			func(pathNum, paths);
		}
	}


}
