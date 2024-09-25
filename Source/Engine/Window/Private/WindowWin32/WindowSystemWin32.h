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

		HINSTANCE m_HAppInst{ nullptr };
		HWND      m_HWnd{ nullptr };
		bool      m_AppPaused{ false };
		bool      m_Minimized{ false };
		bool      m_Maximized{ false };
		bool      m_Resizing{ false };
		USize2D   m_Size{ 0u,0u };

		bool m_FocusMode{ false };

		TArray<OnKeyFunc>         m_OnKeyFunc;
		TArray<OnMouseButtonFunc> m_OnMouseButtonFunc;
		TArray<OnCursorPosFunc>   m_OnCursorPosFunc;
		TArray<OnCursorEnterFunc> m_OnCursorEnterFunc;
		TArray<OnScrollFunc>      m_OnScrollFunc;
		TArray<OnDropFunc>        m_OnDropFunc;
		TArray<OnWindowSizeFunc>  m_OnWindowSizeFunc;
		TArray<OnWindowCloseFunc> m_OnWindowCloseFunc;
		LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); //window loop func
		static LRESULT CALLBACK SMainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void InitKeyButtonCodeMap(); //register key code map
		void OnResize();
		void OnMouseDown(WPARAM btnState, int x, int y);
		void OnMouseUp(WPARAM btnState, int x, int y);
		void OnMouseMove(WPARAM btnState, int x, int y);
	};
}