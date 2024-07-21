#pragma once
#include "Window/Public/EngineWindow.h"
//#include <WinUser.h>
//#include <windef.h>
//#include <d3d11.h>
#include <winapifamily.h>
#include <Windows.h>
#include <unordered_map>


namespace Engine {
	class WindowSystemWin32 final: public EngineWindow {
	private:
		static std::unordered_map<int, EKey> s_GLFWKeyCodeMap;
		static std::unordered_map<int, EBtn> s_GLFWButtonCodeMap;
		static std::unordered_map<int, EInput> s_GLFWInputMap;

		HINSTANCE m_HAppInst{ nullptr };
		HWND      m_MainWnd{ nullptr };
		bool      m_AppPaused{ false };
		bool      m_Minimized{ false };
		bool      m_Maximized{ false };
		bool      m_Resizing{ false };
		USize2D   m_Size{ 0u,0u };

		bool m_FocusMode {false};

		std::vector<OnKeyFunc>         m_OnKeyFunc;
		std::vector<OnMouseButtonFunc> m_OnMouseButtonFunc;
		std::vector<OnCursorPosFunc>   m_OnCursorPosFunc;
		std::vector<OnCursorEnterFunc> m_OnCursorEnterFunc;
		std::vector<OnScrollFunc>      m_OnScrollFunc;
		std::vector<OnDropFunc>        m_OnDropFunc;
		std::vector<OnWindowSizeFunc>  m_OnWindowSizeFunc;
		std::vector<OnWindowCloseFunc> m_OnWindowCloseFunc;
		LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); //window loop func
		static LRESULT CALLBACK SMainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	public:
		explicit WindowSystemWin32(const WindowInitInfo& initInfo);
		~WindowSystemWin32() override;
		void Update() override;
		bool ShouldClose() override;
		void Close() override;
		void SetTitle(const char* title) override;
		const USize2D& GetWindowSize() override;
		void SetFocusMode(bool focusMode) override;
		bool GetFocusMode()override;
		void* GetWindowHandle()override;
		void GetWindowContentScale(float* x, float* y)override;

		void SetWindowIcon(int count, const WindowIcon* icons)override;

		// register func
		void RegisterOnKeyFunc(OnKeyFunc&& func)override;
		void RegisterOnMouseButtonFunc(OnMouseButtonFunc&& func)override;
		void RegisterOnCursorPosFunc(OnCursorPosFunc&& func)override;
		void RegisterOnCursorEnterFunc(OnCursorEnterFunc&& func)override;
		void RegisterOnScrollFunc(OnScrollFunc&& func)override;
		void RegisterOnWindowSizeFunc(OnWindowSizeFunc&& func)override;
		

	private:
		void InitKeyButtonCodeMap(); //register key code map
		void OnResize();
		void OnMouseDown(WPARAM btnState, int x, int y);
		void OnMouseUp(WPARAM btnState, int x, int y);
		void OnMouseMove(WPARAM btnState, int x, int y);
	};
}