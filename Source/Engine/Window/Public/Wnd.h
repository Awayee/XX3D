#pragma once
#include <functional>

#include "InputEnum.h"
#include "Core/Public/BaseStructs.h"


namespace Engine {

    struct WindowInitInfo {
        uint32 width;
        uint32 height;
        const char* title;
        bool resizeable;

    };

    struct WindowIcon {
        int width;
        int height;
        unsigned char* pixels;
    };

	class Wnd {
	public:
		virtual ~Wnd() {};
        virtual void Initialize(const WindowInitInfo& initInfo) = 0;
        virtual void Release() = 0;
		virtual void Tick() = 0;
		virtual bool ShouldClose() = 0;
        virtual void Close() = 0;
		virtual void SetTitle(const char* title) = 0;
        virtual const USize2D& GetWindowSize() = 0;

        typedef std::function<void()>                   OnResetFunc;
        typedef std::function<void(EKey, EInput)>       OnKeyFunc;
        typedef std::function<void(EBtn, EInput)>       OnMouseButtonFunc;
        typedef std::function<void(double, double)>     OnCursorPosFunc;
        typedef std::function<void(int)>                OnCursorEnterFunc;
        typedef std::function<void(double, double)>     OnScrollFunc;
        typedef std::function<void(int, const char**)>  OnDropFunc;
        typedef std::function<void(uint32, uint32)>     OnWindowSizeFunc;
        typedef std::function<void()>                   OnWindowCloseFunc;

        virtual void RegisterOnResetFunc(OnResetFunc&& func) { }
        virtual void RegisterOnKeyFunc(OnKeyFunc&& func) {  }
        virtual void RegisterOnMouseButtonFunc(OnMouseButtonFunc&& func) {  }
        virtual void RegisterOnCursorPosFunc(OnCursorPosFunc&& func) {  }
        virtual void RegisterOnCursorEnterFunc(OnCursorEnterFunc&& func) {  }
        virtual void RegisterOnScrollFunc(OnScrollFunc&& func) {  }
        virtual void RegisterOnWindowSizeFunc(OnWindowSizeFunc&& func) {  }
        virtual void RegisterOnWindowCloseFunc(OnWindowCloseFunc&& func) {  }

        virtual bool GetFocusMode() = 0;
        virtual void* GetWindowHandle() = 0;
        virtual void GetWindowContentScale(float* x, float* y) = 0;

        virtual void SetFocusMode(bool focusMode) = 0;
        virtual void SetWindowIcon(int count, const WindowIcon* icons) = 0;

        static Wnd* Instance();
	};
}