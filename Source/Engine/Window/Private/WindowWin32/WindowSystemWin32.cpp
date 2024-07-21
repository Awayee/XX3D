#include "WindowSystemWin32.h"
#include "Core/Public/Log.h"
#include <windowsx.h>
#include <wingdi.h>

namespace Engine {

	std::unordered_map<int, EKey> WindowSystemWin32::s_GLFWKeyCodeMap;
	std::unordered_map<int, EBtn> WindowSystemWin32::s_GLFWButtonCodeMap;
	std::unordered_map<int, EInput> WindowSystemWin32::s_GLFWInputMap;


	LRESULT WindowSystemWin32::MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg)
		{
			// WM_ACTIVATE is sent when the window is activated or deactivated.  
			// We pause the game when the window is deactivated and unpause it 
			// when it becomes active.  
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				m_AppPaused = true;
			}
			else
			{
				m_AppPaused = false;
			}
			return 0;

			// WM_SIZE is sent when the user resizes the window.  
		case WM_SIZE:
			// Save the new client area dimensions.
			m_Size.w = LOWORD(lParam);
			m_Size.h = HIWORD(lParam);
			{
				if (wParam == SIZE_MINIMIZED)
				{
					m_AppPaused = true;
					m_Minimized = true;
					m_Maximized = false;
				}
				else if (wParam == SIZE_MAXIMIZED)
				{
					m_AppPaused = false;
					m_Minimized = false;
					m_Maximized = true;
					OnResize();
				}
				else if (wParam == SIZE_RESTORED)
				{

					// Restoring from minimized state?
					if (m_Minimized)
					{
						m_AppPaused = false;
						m_Minimized = false;
						OnResize();
					}

					// Restoring from maximized state?
					else if (m_Maximized)
					{
						m_AppPaused = false;
						m_Maximized = false;
						OnResize();
					}
					else if (m_Resizing)
					{
						// If user is dragging the resize bars, we do not resize 
						// the buffers here because as the user continuously 
						// drags the resize bars, a stream of WM_SIZE messages are
						// sent to the window, and it would be pointless (and slow)
						// to resize for each WM_SIZE message received from dragging
						// the resize bars.  So instead, we reset after the user is 
						// done resizing the window and releases the resize bars, which 
						// sends a WM_EXITSIZEMOVE message.
					}
					else // API call such as SetWindowPos or mSwapchain->SetFullscreenState.
					{
						OnResize();
					}
				}
			}
			return 0;

			// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
		case WM_ENTERSIZEMOVE:
			m_AppPaused = true;
			m_Resizing = true;
			return 0;

			// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
			// Here we reset everything based on the new window dimensions.
		case WM_EXITSIZEMOVE:
			m_AppPaused = false;
			m_Resizing = false;
			OnResize();
			return 0;

			// WM_DESTROY is sent when the window is being destroyed.
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

			// The WM_MENUCHAR message is sent when a menu is active and the user presses 
			// a key that does not correspond to any mnemonic or accelerator key. 
		case WM_MENUCHAR:
			// Don't beep when we alt-enter.
			return MAKELRESULT(0, MNC_CLOSE);

			// Catch this message so to prevent the window from becoming too small.
		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
			return 0;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_MOUSEMOVE:
			OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	LRESULT WindowSystemWin32::SMainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return dynamic_cast<WindowSystemWin32*>(Instance())->MainWndProc(hwnd, msg, wParam, lParam);
	}

	WindowSystemWin32::WindowSystemWin32(const WindowInitInfo& initInfo) {
		WNDCLASS wc;
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WindowSystemWin32::SMainWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = m_HAppInst;
		wc.hIcon = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		wc.lpszMenuName = 0;
		wc.lpszClassName = initInfo.title;
		if (!RegisterClass(&wc)) {
			MessageBox(nullptr, "RegisterClass Failed.", nullptr, 0);
			ASSERT(0, "");
		}
		RECT R = { 0, 0, initInfo.width, initInfo.height };
		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
		int width = R.right - R.left;
		int height = R.bottom - R.top;
		m_MainWnd = CreateWindow(initInfo.title, initInfo.title,
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_HAppInst, 0);
		if (!m_MainWnd) {
			MessageBox(0, "CreateWindow Failed.", 0, 0);
			ASSERT(0, "");
		}
		ShowWindow(m_MainWnd, SW_SHOW);
		UpdateWindow(m_MainWnd);
	}

	WindowSystemWin32::~WindowSystemWin32(){
	}

	void WindowSystemWin32::Update(){

	}
	bool WindowSystemWin32::ShouldClose(){
		return false;
	}
	void WindowSystemWin32::Close(){
	}
	void WindowSystemWin32::SetTitle(const char* title){
	}

	const USize2D& WindowSystemWin32::GetWindowSize() {
		return m_Size;
	}

	void WindowSystemWin32::SetFocusMode(bool focusMode){
		m_FocusMode = focusMode;
	}

	bool WindowSystemWin32::GetFocusMode(){
		return m_FocusMode;
	}

	void* WindowSystemWin32::GetWindowHandle(){
		return (void*)m_MainWnd;
	}

	void WindowSystemWin32::GetWindowContentScale(float* x, float* y){
	}

	void WindowSystemWin32::SetWindowIcon(int count, const WindowIcon* icons){
	}

	void WindowSystemWin32::RegisterOnMouseButtonFunc(OnMouseButtonFunc&& func)
	{
		m_OnMouseButtonFunc.push_back(func);
	}

	void WindowSystemWin32::RegisterOnCursorPosFunc(OnCursorPosFunc&& func)
	{
		m_OnCursorPosFunc.push_back(func);
	}

	void WindowSystemWin32::RegisterOnCursorEnterFunc(OnCursorEnterFunc&& func)
	{
		m_OnCursorEnterFunc.push_back(func);
	}

	void WindowSystemWin32::RegisterOnScrollFunc(OnScrollFunc&& func)
	{
		m_OnScrollFunc.push_back(func);
	}

	void WindowSystemWin32::RegisterOnWindowSizeFunc(OnWindowSizeFunc&& func)
	{
		m_OnWindowSizeFunc.push_back(func);
	}

	void WindowSystemWin32::RegisterOnKeyFunc(OnKeyFunc&& func)
	{
		m_OnKeyFunc.push_back(func);
	}



	// There is no distinct VK_xxx for keypad enter, instead it is VK_RETURN + KF_EXTENDED, we assign it an arbitrary value to make code more readable (VK_ codes go up to 255)
#define IM_VK_KEYPAD_ENTER      (VK_RETURN + 256)
	void WindowSystemWin32::InitKeyButtonCodeMap()
	{
		s_GLFWKeyCodeMap[VK_SPACE] = EKey::SPACE;
		s_GLFWKeyCodeMap[VK_OEM_7] = EKey::APOSTROPHE; /* ' */
		s_GLFWKeyCodeMap[VK_OEM_COMMA] = EKey::COMMA; /* ; */
		s_GLFWKeyCodeMap[VK_OEM_MINUS] = EKey::MINUS; /* - */
		s_GLFWKeyCodeMap[VK_OEM_PERIOD] = EKey::PERIOD; /* . */
		s_GLFWKeyCodeMap[VK_OEM_2] = EKey::SLASH; /* / */
		s_GLFWKeyCodeMap[VK_OEM_1] = EKey::SEMICOLON; /* ; */
		s_GLFWKeyCodeMap[VK_OEM_PLUS] = EKey::EQUAL; /* = */
		s_GLFWKeyCodeMap['0'] = EKey::A0;
		s_GLFWKeyCodeMap['1'] = EKey::A1;
		s_GLFWKeyCodeMap['2'] = EKey::A2;
		s_GLFWKeyCodeMap['3'] = EKey::A3;
		s_GLFWKeyCodeMap['4'] = EKey::A4;
		s_GLFWKeyCodeMap['5'] = EKey::A5;
		s_GLFWKeyCodeMap['6'] = EKey::A6;
		s_GLFWKeyCodeMap['7'] = EKey::A7;
		s_GLFWKeyCodeMap['8'] = EKey::A8;
		s_GLFWKeyCodeMap['9'] = EKey::A9;
		s_GLFWKeyCodeMap['A'] = EKey::A;
		s_GLFWKeyCodeMap['B'] = EKey::B;
		s_GLFWKeyCodeMap['C'] = EKey::C;
		s_GLFWKeyCodeMap['D'] = EKey::D;
		s_GLFWKeyCodeMap['E'] = EKey::E;
		s_GLFWKeyCodeMap['F'] = EKey::F;
		s_GLFWKeyCodeMap['G'] = EKey::G;
		s_GLFWKeyCodeMap['H'] = EKey::H;
		s_GLFWKeyCodeMap['I'] = EKey::I;
		s_GLFWKeyCodeMap['J'] = EKey::J;
		s_GLFWKeyCodeMap['K'] = EKey::K;
		s_GLFWKeyCodeMap['L'] = EKey::L;
		s_GLFWKeyCodeMap['M'] = EKey::M;
		s_GLFWKeyCodeMap['N'] = EKey::N;
		s_GLFWKeyCodeMap['O'] = EKey::O;
		s_GLFWKeyCodeMap['P'] = EKey::P;
		s_GLFWKeyCodeMap['Q'] = EKey::Q;
		s_GLFWKeyCodeMap['R'] = EKey::R;
		s_GLFWKeyCodeMap['S'] = EKey::S;
		s_GLFWKeyCodeMap['T'] = EKey::T;
		s_GLFWKeyCodeMap['U'] = EKey::U;
		s_GLFWKeyCodeMap['V'] = EKey::V;
		s_GLFWKeyCodeMap['W'] = EKey::W;
		s_GLFWKeyCodeMap['X'] = EKey::X;
		s_GLFWKeyCodeMap['Y'] = EKey::Y;
		s_GLFWKeyCodeMap['Z'] = EKey::Z;
		s_GLFWKeyCodeMap[VK_OEM_4] = EKey::LEFT_BRACKET; /* [ */
		s_GLFWKeyCodeMap[VK_OEM_5] = EKey::BACKSLASH; /* \ */
		s_GLFWKeyCodeMap[VK_OEM_6] = EKey::RIGHT_BRACKET; /* ] */
		s_GLFWKeyCodeMap[VK_OEM_3] = EKey::GRAVE_ACCENT; /* ` */
		//s_GLFWKeyCodeMap[GLFW_KEY_WORLD_1] = EKeyCode::WORLD_1; /* non-US #1 */
		//s_GLFWKeyCodeMap[GLFW_KEY_WORLD_2] = EKeyCode::WORLD_2; /* non-US #2 */
		s_GLFWKeyCodeMap[VK_ESCAPE] = EKey::ESCAPE;
		s_GLFWKeyCodeMap[VK_RETURN] = EKey::ENTER;
		s_GLFWKeyCodeMap[VK_TAB] = EKey::TAB;
		s_GLFWKeyCodeMap[VK_BACK] = EKey::BACKSPACE;
		s_GLFWKeyCodeMap[VK_INSERT] = EKey::INSERT;
		s_GLFWKeyCodeMap[VK_DELETE] = EKey::DEL;
		s_GLFWKeyCodeMap[VK_RIGHT] = EKey::RIGHT;
		s_GLFWKeyCodeMap[VK_LEFT] = EKey::LEFT;
		s_GLFWKeyCodeMap[VK_DOWN] = EKey::DOWN;
		s_GLFWKeyCodeMap[VK_UP] = EKey::UP;
		s_GLFWKeyCodeMap[VK_PRIOR] = EKey::PAGE_UP;
		s_GLFWKeyCodeMap[VK_NEXT] = EKey::PAGE_DOWN;
		s_GLFWKeyCodeMap[VK_HOME] = EKey::HOME;
		s_GLFWKeyCodeMap[VK_END] = EKey::END;
		s_GLFWKeyCodeMap[VK_CAPITAL] = EKey::CAPS_LOCK;
		s_GLFWKeyCodeMap[VK_SCROLL] = EKey::SCROLL_LOCK;
		s_GLFWKeyCodeMap[VK_NUMLOCK] = EKey::NUM_LOCK;
		s_GLFWKeyCodeMap[VK_SNAPSHOT] = EKey::PRINT_SCREEN;
		s_GLFWKeyCodeMap[VK_PAUSE] = EKey::PAUSE;
		s_GLFWKeyCodeMap[VK_F1] = EKey::F1;
		s_GLFWKeyCodeMap[VK_F2] = EKey::F2;
		s_GLFWKeyCodeMap[VK_F3] = EKey::F3;
		s_GLFWKeyCodeMap[VK_F4] = EKey::F4;
		s_GLFWKeyCodeMap[VK_F5] = EKey::F5;
		s_GLFWKeyCodeMap[VK_F6] = EKey::F6;
		s_GLFWKeyCodeMap[VK_F7] = EKey::F7;
		s_GLFWKeyCodeMap[VK_F8] = EKey::F8;
		s_GLFWKeyCodeMap[VK_F9] = EKey::F9;
		s_GLFWKeyCodeMap[VK_F10] = EKey::F10;
		s_GLFWKeyCodeMap[VK_F11] = EKey::F11;
		s_GLFWKeyCodeMap[VK_F12] = EKey::F12;
		s_GLFWKeyCodeMap[VK_NUMPAD0] = EKey::KP_0;
		s_GLFWKeyCodeMap[VK_NUMPAD1] = EKey::KP_1;
		s_GLFWKeyCodeMap[VK_NUMPAD2] = EKey::KP_2;
		s_GLFWKeyCodeMap[VK_NUMPAD3] = EKey::KP_3;
		s_GLFWKeyCodeMap[VK_NUMPAD4] = EKey::KP_4;
		s_GLFWKeyCodeMap[VK_NUMPAD5] = EKey::KP_5;
		s_GLFWKeyCodeMap[VK_NUMPAD6] = EKey::KP_6;
		s_GLFWKeyCodeMap[VK_NUMPAD7] = EKey::KP_7;
		s_GLFWKeyCodeMap[VK_NUMPAD8] = EKey::KP_8;
		s_GLFWKeyCodeMap[VK_NUMPAD9] = EKey::KP_9;
		s_GLFWKeyCodeMap[VK_DECIMAL] = EKey::KP_DECIMAL;
		s_GLFWKeyCodeMap[VK_DIVIDE] = EKey::KP_DIVIDE;
		s_GLFWKeyCodeMap[VK_MULTIPLY] = EKey::KP_MULTIPLY;
		s_GLFWKeyCodeMap[VK_SUBTRACT] = EKey::KP_SUBTRACT;
		s_GLFWKeyCodeMap[VK_ADD] = EKey::KP_ADD;
		s_GLFWKeyCodeMap[IM_VK_KEYPAD_ENTER] = EKey::KP_ENTER;
		//s_GLFWKeyCodeMap[IM_VK_KEYPAD_ENTER] = EKeyCode::KP_EQUAL;
		s_GLFWKeyCodeMap[VK_LSHIFT] = EKey::LEFT_SHIFT;
		s_GLFWKeyCodeMap[VK_LCONTROL] = EKey::LEFT_CONTROL;
		s_GLFWKeyCodeMap[VK_LMENU] = EKey::LEFT_ALT;
		s_GLFWKeyCodeMap[VK_LWIN] = EKey::LEFT_SUPER;
		s_GLFWKeyCodeMap[VK_RSHIFT] = EKey::RIGHT_SHIFT;
		s_GLFWKeyCodeMap[VK_RCONTROL] = EKey::RIGHT_CONTROL;
		s_GLFWKeyCodeMap[VK_RMENU] = EKey::RIGHT_ALT;
		s_GLFWKeyCodeMap[VK_RWIN] = EKey::RIGHT_SUPER;
		s_GLFWKeyCodeMap[VK_MENU] = EKey::MENU;

		s_GLFWButtonCodeMap[VK_LBUTTON] = EBtn::LEFT;
		s_GLFWButtonCodeMap[VK_RBUTTON] = EBtn::RIGHT;
		s_GLFWButtonCodeMap[VK_MBUTTON] = EBtn::MIDDLE;
		s_GLFWButtonCodeMap[VK_XBUTTON1] = EBtn::BTN_4;
		s_GLFWButtonCodeMap[VK_XBUTTON2] = EBtn::BTN_5;
	}

	void WindowSystemWin32::OnResize() {
		for(auto& func: m_OnWindowSizeFunc) {
			func(m_Size.w, m_Size.h);
		}
	}

	void WindowSystemWin32::OnMouseDown(WPARAM btnState, int x, int y) {
	}

	void WindowSystemWin32::OnMouseUp(WPARAM btnState, int x, int y) {
	}

	void WindowSystemWin32::OnMouseMove(WPARAM btnState, int x, int y) {
	}
}
