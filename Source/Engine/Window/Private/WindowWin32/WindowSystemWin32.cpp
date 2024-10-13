#include "WindowSystemWin32.h"
#include "Core/Public/Log.h"
#include "Core/Public/String.h"
#include "Engine/Public/Engine.h"
#include <windowsx.h>
#include <wingdi.h>
#include <imgui_internal.h>

// There is no distinct VK_xxx for keypad enter, instead it is VK_RETURN + KF_EXTENDED, we assign it an arbitrary value to make code more readable (VK_ codes go up to 255)
#define IM_VK_KEYPAD_ENTER (VK_RETURN + 256)
constexpr uint16 KEY_STATE_PRESSING = 1 << 15;
constexpr uint16 KEY_STATE_PRESSED = 1;
constexpr uint64 WN_KEY_STATE_REPEAT = 1 << 30;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Engine {
	WindowSystemWin32::WindowSystemWin32() {
		m_HAppInst = GetModuleHandle(nullptr);
	}

	WindowSystemWin32::~WindowSystemWin32(){
	}

	void WindowSystemWin32::Initialize(const WindowInitInfo& initInfo) {
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
		wc.lpszClassName = initInfo.Title.c_str();
		if (!RegisterClass(&wc)) {
			LOG_ERROR("RegisterClass Failed.");
		}
		LONG dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		if(initInfo.Resizeable) {
			dwStyle |= WS_THICKFRAME | WS_MAXIMIZEBOX;
		}
		RECT R = { 0, 0, (LONG)initInfo.Width, (LONG)initInfo.Height };
		AdjustWindowRect(&R, dwStyle, false);
		int width = R.right - R.left;
		int height = R.bottom - R.top;
		m_HWnd = CreateWindow(initInfo.Title.c_str(), initInfo.Title.c_str(),
			dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_HAppInst, 0);
		if (!m_HWnd) {
			LOG_ERROR("CreateWindow Failed.");
		}
		// handle dpi scale
		BOOL result = SetProcessDPIAware();
		if (!result) {
			CHECK(0);
		}
		ShowWindow(m_HWnd, SW_SHOWDEFAULT);
		UpdateWindow(m_HWnd);
		InitKeyButtonCodeMap();
	}

	void WindowSystemWin32::Update(){
		MSG msg = { 0 };
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)){
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT || msg.message == WM_CLOSE) {
				Engine::XXEngine::ShutDown();
				return;
			}
		}
	}

	void WindowSystemWin32::Close(){
		CloseWindow(m_HWnd);
	}

	void WindowSystemWin32::SetTitle(const char* title){
		SetWindowText(m_HWnd, title);
	}

	void WindowSystemWin32::SetWindowIcon(int count, const WindowIcon* icons) {
	}

	bool WindowSystemWin32::GetFocusMode(){
		return ::GetForegroundWindow() == m_HWnd;
	}

	void* WindowSystemWin32::GetWindowHandle(){
		return (void*)m_HWnd;
	}

	USize2D WindowSystemWin32::GetWindowSize() {
		return m_Size;
	}

	FSize2D WindowSystemWin32::GetWindowContentScale() {
		const uint32 dpi = GetDpiForWindow(m_HWnd);
		const float contentScale = (float)dpi / 96.0f;
		return { contentScale, contentScale };
	}

	FOffset2D WindowSystemWin32::GetCursorPos() {
		POINT cursorPos;
		::GetCursorPos(&cursorPos);
		::ScreenToClient(m_HWnd, &cursorPos);
		return { (float)cursorPos.x, (float)cursorPos.y };
	}

	bool WindowSystemWin32::IsKeyDown(EKey key) {
		return GetFocusMode() && GetAsyncKeyState(m_KeyMap.GetNum(key)) & KEY_STATE_PRESSING;
	}

	bool WindowSystemWin32::IsMouseDown(EBtn btn) {
		return GetFocusMode() && GetAsyncKeyState(m_MouseButtonMap.GetNum(btn)) & KEY_STATE_PRESSING;
	}

	void WindowSystemWin32::RegisterOnDropFunc(OnDropFunc&& func) {
		EngineWindow::RegisterOnDropFunc(MoveTemp(func));
		DragAcceptFiles(m_HWnd, TRUE);
	}

	LRESULT WindowSystemWin32::MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
		switch (msg){
		case WM_CREATE:
			// WM_ACTIVATE is sent when the window is activated or deactivated.  
			// We pause the game when the window is deactivated and unpause it 
			// when it becomes active.
			return 0;
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE){
				m_AppPaused = true;
			}
			else{
				m_AppPaused = false;
			}
			return 0;

			// WM_SIZE is sent when the user resizes the window.  
		case WM_SIZE:
			// Save the new client area dimensions.
			m_Size.w = LOWORD(lParam);
			m_Size.h = HIWORD(lParam);{
				if (wParam == SIZE_MINIMIZED){
					m_AppPaused = true;
					m_Minimized = true;
					m_Maximized = false;
				}
				else if (wParam == SIZE_MAXIMIZED){
					m_AppPaused = false;
					m_Minimized = false;
					m_Maximized = true;
					OnResize();
				}
				else if (wParam == SIZE_RESTORED){
					// Restoring from minimized state?
					if (m_Minimized){
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
					else if (m_Resizing){
						// If user is dragging the resize bars, we do not resize 
						// the buffers here because as the user continuously 
						// drags the resize bars, a stream of WM_SIZE messages are
						// sent to the window, and it would be pointless (and slow)
						// to resize for each WM_SIZE message received from dragging
						// the resize bars.  So instead, we reset after the user is 
						// done resizing the window and releases the resize bars, which 
						// sends a WM_EXITSIZEMOVE message.
					}
					else{
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
			OnMouseButtonDown(EBtn::Left);
			return 0;
		case WM_LBUTTONUP:
			OnMouseButtonUp(EBtn::Left);
			return 0;
		case WM_RBUTTONDOWN:
			OnMouseButtonDown(EBtn::Right);
			return 0;
		case WM_RBUTTONUP:
			OnMouseButtonUp(EBtn::Right);
			return 0;
		case WM_MBUTTONDOWN:
			OnMouseButtonDown(EBtn::Middle);
			return 0;
		case WM_MBUTTONUP:
			OnMouseButtonUp(EBtn::Middle);
			return 0;
		case WM_MOUSEMOVE:
			OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_KEYDOWN:
			if (lParam & WN_KEY_STATE_REPEAT) {
				OnKeyRepeat((int)wParam);
			}
			else {
				OnKeyDown((int)wParam);
			}
			return 0;
		case WM_KEYUP:
			OnKeyUp((int)wParam);
			return 0;
		case WM_SETFOCUS:
			OnFocus(false);
			return 0;
		case WM_KILLFOCUS:
			OnFocus(true);
			return 0;
		case WM_MOUSEWHEEL:
			OnScroll(GET_WHEEL_DELTA_WPARAM(wParam));
			return 0;
		case WM_DROPFILES:
			HDROP hDrop = (HDROP)wParam;
			UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
			TArray<const char*> paths(fileCount);
			TArray<XString> pathStr(fileCount);
			for (UINT i = 0; i < fileCount; ++i) {
				pathStr[i].resize(MAX_PATH);
				DragQueryFile(hDrop, i, pathStr[i].data(), MAX_PATH);
				paths[i] = pathStr[i].c_str();
			}
			OnDrop((int)fileCount, paths.Data());
			DragFinish(hDrop);
			return 0;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	LRESULT WindowSystemWin32::SMainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return static_cast<WindowSystemWin32*>(Instance())->MainWndProc(hwnd, msg, wParam, lParam);
	}

	void WindowSystemWin32::InitKeyButtonCodeMap(){
#define PARSE_KEY_CODE(win32Key, key) m_KeyMap.AddPair(win32Key, key)
		PARSE_KEY_CODE(VK_SPACE, EKey::Space);
		PARSE_KEY_CODE(VK_OEM_7, EKey::Apostrophe); /* ' */
		PARSE_KEY_CODE(VK_OEM_COMMA, EKey::Comma); /* ; */
		PARSE_KEY_CODE(VK_OEM_MINUS, EKey::Minus); /* - */
		PARSE_KEY_CODE(VK_OEM_PERIOD, EKey::Period); /* . */
		PARSE_KEY_CODE(VK_OEM_2, EKey::Slash); /* / */
		PARSE_KEY_CODE(VK_OEM_1, EKey::Semicolon); /* ; */
		PARSE_KEY_CODE(VK_OEM_PLUS, EKey::Equal); /* = */
		PARSE_KEY_CODE('0', EKey::A0);
		PARSE_KEY_CODE('1', EKey::A1);
		PARSE_KEY_CODE('2', EKey::A2);
		PARSE_KEY_CODE('3', EKey::A3);
		PARSE_KEY_CODE('4', EKey::A4);
		PARSE_KEY_CODE('5', EKey::A5);
		PARSE_KEY_CODE('6', EKey::A6);
		PARSE_KEY_CODE('7', EKey::A7);
		PARSE_KEY_CODE('8', EKey::A8);
		PARSE_KEY_CODE('9', EKey::A9);
		PARSE_KEY_CODE('A', EKey::A);
		PARSE_KEY_CODE('B', EKey::B);
		PARSE_KEY_CODE('C', EKey::C);
		PARSE_KEY_CODE('D', EKey::D);
		PARSE_KEY_CODE('E', EKey::E);
		PARSE_KEY_CODE('F', EKey::F);
		PARSE_KEY_CODE('G', EKey::G);
		PARSE_KEY_CODE('H', EKey::H);
		PARSE_KEY_CODE('I', EKey::I);
		PARSE_KEY_CODE('J', EKey::J);
		PARSE_KEY_CODE('K', EKey::K);
		PARSE_KEY_CODE('L', EKey::L);
		PARSE_KEY_CODE('M', EKey::M);
		PARSE_KEY_CODE('N', EKey::N);
		PARSE_KEY_CODE('O', EKey::O);
		PARSE_KEY_CODE('P', EKey::P);
		PARSE_KEY_CODE('Q', EKey::Q);
		PARSE_KEY_CODE('R', EKey::R);
		PARSE_KEY_CODE('S', EKey::S);
		PARSE_KEY_CODE('T', EKey::T);
		PARSE_KEY_CODE('U', EKey::U);
		PARSE_KEY_CODE('V', EKey::V);
		PARSE_KEY_CODE('W', EKey::W);
		PARSE_KEY_CODE('X', EKey::X);
		PARSE_KEY_CODE('Y', EKey::Y);
		PARSE_KEY_CODE('Z', EKey::Z);
		PARSE_KEY_CODE(VK_OEM_4, EKey::LeftBracket); /* [ */
		PARSE_KEY_CODE(VK_OEM_5, EKey::BackSlash); /* \ */
		PARSE_KEY_CODE(VK_OEM_6, EKey::RightBracket); /* ] */
		PARSE_KEY_CODE(VK_OEM_3, EKey::GraveAccent); /* ` */
		//PARSE_KEY_CODE(VK_WORLD_1, EKeyCode::World1); /* non-US #1 */
		//PARSE_KEY_CODE(VK_WORLD_2, EKeyCode::World2); /* non-US #2 */
		PARSE_KEY_CODE(VK_ESCAPE, EKey::Escape);
		PARSE_KEY_CODE(VK_RETURN, EKey::Enter);
		PARSE_KEY_CODE(VK_TAB, EKey::Tab);
		PARSE_KEY_CODE(VK_BACK, EKey::Backspace);
		PARSE_KEY_CODE(VK_INSERT, EKey::Insert);
		PARSE_KEY_CODE(VK_DELETE, EKey::Delete);
		PARSE_KEY_CODE(VK_RIGHT, EKey::Right);
		PARSE_KEY_CODE(VK_LEFT, EKey::Left);
		PARSE_KEY_CODE(VK_DOWN, EKey::Down);
		PARSE_KEY_CODE(VK_UP, EKey::Up);
		PARSE_KEY_CODE(VK_PRIOR, EKey::PageUp);
		PARSE_KEY_CODE(VK_NEXT, EKey::PageDown);
		PARSE_KEY_CODE(VK_HOME, EKey::Home);
		PARSE_KEY_CODE(VK_END, EKey::End);
		PARSE_KEY_CODE(VK_CAPITAL, EKey::CapsLock);
		PARSE_KEY_CODE(VK_SCROLL, EKey::ScrollLock);
		PARSE_KEY_CODE(VK_NUMLOCK, EKey::NumLock);
		PARSE_KEY_CODE(VK_SNAPSHOT, EKey::PrtSc);
		PARSE_KEY_CODE(VK_PAUSE, EKey::Pause);
		PARSE_KEY_CODE(VK_F1, EKey::F1);
		PARSE_KEY_CODE(VK_F2, EKey::F2);
		PARSE_KEY_CODE(VK_F3, EKey::F3);
		PARSE_KEY_CODE(VK_F4, EKey::F4);
		PARSE_KEY_CODE(VK_F5, EKey::F5);
		PARSE_KEY_CODE(VK_F6, EKey::F6);
		PARSE_KEY_CODE(VK_F7, EKey::F7);
		PARSE_KEY_CODE(VK_F8, EKey::F8);
		PARSE_KEY_CODE(VK_F9, EKey::F9);
		PARSE_KEY_CODE(VK_F10, EKey::F10);
		PARSE_KEY_CODE(VK_F11, EKey::F11);
		PARSE_KEY_CODE(VK_F12, EKey::F12);
		PARSE_KEY_CODE(VK_NUMPAD0, EKey::KP0);
		PARSE_KEY_CODE(VK_NUMPAD1, EKey::KP1);
		PARSE_KEY_CODE(VK_NUMPAD2, EKey::KP2);
		PARSE_KEY_CODE(VK_NUMPAD3, EKey::KP3);
		PARSE_KEY_CODE(VK_NUMPAD4, EKey::KP4);
		PARSE_KEY_CODE(VK_NUMPAD5, EKey::KP5);
		PARSE_KEY_CODE(VK_NUMPAD6, EKey::KP6);
		PARSE_KEY_CODE(VK_NUMPAD7, EKey::KP7);
		PARSE_KEY_CODE(VK_NUMPAD8, EKey::KP8);
		PARSE_KEY_CODE(VK_NUMPAD9, EKey::KP9);
		PARSE_KEY_CODE(VK_DECIMAL, EKey::KPDecimal);
		PARSE_KEY_CODE(VK_DIVIDE, EKey::KPDivide);
		PARSE_KEY_CODE(VK_MULTIPLY, EKey::KPMultiply);
		PARSE_KEY_CODE(VK_SUBTRACT, EKey::KPSubtract);
		PARSE_KEY_CODE(VK_ADD, EKey::KPAdd);
		PARSE_KEY_CODE(IM_VK_KEYPAD_ENTER, EKey::KPEnter);
		PARSE_KEY_CODE(VK_LSHIFT, EKey::LeftShit);
		PARSE_KEY_CODE(VK_LCONTROL, EKey::LeftCtrl);
		PARSE_KEY_CODE(VK_LMENU, EKey::LeftAlt);
		PARSE_KEY_CODE(VK_LWIN, EKey::LeftSuper);
		PARSE_KEY_CODE(VK_RSHIFT, EKey::RightShift);
		PARSE_KEY_CODE(VK_RCONTROL, EKey::RightCtrl);
		PARSE_KEY_CODE(VK_RMENU, EKey::RightAlt);
		PARSE_KEY_CODE(VK_RWIN, EKey::RightSuper);
		PARSE_KEY_CODE(VK_MENU, EKey::Menu);
#undef PARSE_KEY_CODE

#define PARSE_BTN_CODE(win32Key, key) m_MouseButtonMap.AddPair(win32Key, key)
		PARSE_BTN_CODE(VK_LBUTTON, EBtn::Left);
		PARSE_BTN_CODE(VK_RBUTTON, EBtn::Right);
		PARSE_BTN_CODE(VK_MBUTTON, EBtn::Middle);
		PARSE_BTN_CODE(VK_XBUTTON1, EBtn::Btn4);
		PARSE_BTN_CODE(VK_XBUTTON2, EBtn::Btn5);
#undef PARSE_BTN_CODE
	}

	void WindowSystemWin32::OnResize() {
		for(auto& func: m_OnWindowSizeFunc) {
			func(m_Size.w, m_Size.h);
		}
	}

	void WindowSystemWin32::OnMouseButtonDown(Engine::EBtn btn) {
		for(auto& func: m_OnMouseButtonFunc) {
			func(btn, EInput::Press);
		}
	}

	void WindowSystemWin32::OnMouseButtonUp(Engine::EBtn btn) {
		for (auto& func : m_OnMouseButtonFunc) {
			func(btn, EInput::Release);
		}
	}

	void WindowSystemWin32::OnMouseMove(int x, int y) {
		for(auto& func: m_OnCursorPosFunc) {
			func((float)x, (float)y);
		}
	}

	void WindowSystemWin32::OnKeyDown(int key) {
		EKey eKey = m_KeyMap.GetEnum(key);
		for(auto& func: m_OnKeyFunc) {
			func(eKey, EInput::Press);
		}
	}

	void WindowSystemWin32::OnKeyUp(int key) {
		EKey eKey = m_KeyMap.GetEnum(key);
		for (auto& func : m_OnKeyFunc) {
			func(eKey, EInput::Release);
		}
	}

	void WindowSystemWin32::OnKeyRepeat(int key) {
		EKey eKey = m_KeyMap.GetEnum(key);
		for(auto& func: m_OnKeyFunc) {
			func(eKey, EInput::Repeat);
		}
	}

	void WindowSystemWin32::OnFocus(bool isFocused) {
		for(auto& func: m_OnWindowFocusFunc) {
			func(isFocused);
		}
	}

	void WindowSystemWin32::OnScroll(int val) {
		for(auto& func: m_OnScrollFunc) {
			func(0, (float)val / WHEEL_DELTA);
		}
	}

	void WindowSystemWin32::OnDrop(int num, const char** paths) {
		for(auto& func: m_OnDropFunc) {
			func(num, paths);
		}
	}

}
