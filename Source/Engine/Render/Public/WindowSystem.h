#pragma once
#include <functional>

#include "InputEnum.h"


namespace Engine {

    struct WindowInitInfo {
        int width;
        int height;
        const char* title;
        bool resizeable;

    };

    struct WindowIcon {
        int width;
        int height;
        unsigned char* pixels;
    };

	class WindowSystemBase {
	public:
		virtual ~WindowSystemBase() {};
        virtual void Initialize(const WindowInitInfo& initInfo) = 0;
		virtual void PollEvents() = 0;
		virtual bool ShouldClose() = 0;
        virtual void Close() = 0;
		virtual void SetTitle(const char* title) = 0;
		virtual void GetWindowSize(int* w, int* h) = 0;

        typedef std::function<void()>                   OnResetFunc;
        typedef std::function<void(EKeyCode, EInputType)> OnKeyFunc;
        typedef std::function<void(EButtonCode, EInputType)>      OnMouseButtonFunc;
        typedef std::function<void(double, double)>     OnCursorPosFunc;
        typedef std::function<void(int)>                OnCursorEnterFunc;
        typedef std::function<void(double, double)>     OnScrollFunc;
        typedef std::function<void(int, const char**)>  OnDropFunc;
        typedef std::function<void(int, int)>           OnWindowSizeFunc;
        typedef std::function<void()>                   OnWindowCloseFunc;

        //typedef void(*OnResetFunc)();
        //typedef void(*OnKeyFunc)(EKeyCode, EInputType);
        //typedef void(*OnMouseButtonFunc)(EButtonCode, EInputType);
        //typedef void(*OnCursorPosFunc)(double, double);
        //typedef void(*OnCursorEnterFunc)(bool);
        //typedef void(*OnScrollFunc)(double, double);
        //typedef void(*OnDropFunc)(int, const char**);
        //typedef void(*OnWindowSizeFunc)(int, int);
        //typedef void(*OnWindowCloseFunc)();

        virtual void RegisterOnResetFunc(OnResetFunc func) { }
        virtual void RegisterOnKeyFunc(OnKeyFunc func) {  }
        virtual void RegisterOnMouseButtonFunc(OnMouseButtonFunc func) {  }
        virtual void RegisterOnCursorPosFunc(OnCursorPosFunc func) {  }
        virtual void RegisterOnCursorEnterFunc(OnCursorEnterFunc func) {  }
        virtual void RegisterOnScrollFunc(OnScrollFunc func) {  }
        virtual void RegisterOnWindowSizeFunc(OnWindowSizeFunc func) {  }
        virtual void RegisterOnWindowCloseFunc(OnWindowCloseFunc func) {  }

        virtual bool GetFocusMode() = 0;
        virtual void* GetWindowHandle() = 0;
        virtual void GetWindowContentScale(float* x, float* y) = 0;

        virtual void SetFocusMode(bool focusMode) = 0;
        virtual void SetWindowIcon(int count, const WindowIcon* icons) = 0;
	};
}