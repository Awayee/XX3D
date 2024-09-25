#pragma once
#include "Core/Public/Container.h"
#include "Core/Public/TArray.h"
#include "Window/Public/EngineWindow.h"
#include <winapifamily.h>
#include <Windows.h>

namespace Engine {
	class WindowSystemWin32 final: public EngineWindow {
	public:
		WindowSystemWin32();
		~WindowSystemWin32() override;
		void Initialize(const WindowInitInfo& initInfo);
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
		TInputBidirectionalMap<VK_LBUTTON, VK_XBUTTON2, EBtn> m_MouseButtonMap;
		HINSTANCE m_HAppInst{ nullptr };
		HWND      m_HWnd{ nullptr };
		bool      m_AppPaused{ false };
		bool      m_Minimized{ false };
		bool      m_Maximized{ false };
		bool      m_Resizing{ false };
		USize2D   m_Size{ 0u,0u };
		LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); //window loop func
		static LRESULT CALLBACK SMainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void InitKeyButtonCodeMap(); //register key code map
		void OnResize();
		void OnMouseButtonDown(Engine::EBtn btn);
		void OnMouseButtonUp(Engine::EBtn btn);
		void OnMouseMove(int x, int y);
		void OnKeyDown(int key);
		void OnKeyUp(int key);
		void OnKeyRepeat(int key);
		void OnFocus(bool isFocused);
		void OnScroll(int val);
		void OnDrag(int num, const char** paths);
	};
}