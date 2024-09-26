#pragma once
#include "InputEnum.h"
#include "Core/Public/BaseStructs.h"
#include "Core/Public/Func.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/TArray.h"
#include "Core/Public/Container.h"
#include "Core/Public/EnumClass.h"

namespace Engine {

    struct WindowInitInfo {
        uint32 Width;
        uint32 Height;
        const char* Title;
        bool Resizeable;

    };

    struct WindowIcon {
        int Width;
        int Height;
        unsigned char* Pixels;
    };

	class EngineWindow {
	public:
        static void Initialize();
        static void Release();
        static EngineWindow* Instance();

		virtual ~EngineWindow() {}
		virtual void Update() = 0;
        virtual void Close() = 0;
		virtual void SetTitle(const char* title) = 0;
        virtual void SetWindowIcon(int count, const WindowIcon* icons) = 0;

        virtual bool GetFocusMode() = 0;
        virtual void* GetWindowHandle() = 0;
        virtual USize2D GetWindowSize() = 0;
        virtual FSize2D GetWindowContentScale() = 0;
        virtual FOffset2D GetCursorPos() = 0;
        virtual bool IsKeyDown(EKey key) = 0; // whether key is pressing state
        virtual bool IsMouseDown(EBtn btn) = 0;

        typedef Func<void(EKey, EInput)>       OnKeyFunc;
        typedef Func<void(EBtn, EInput)>       OnMouseButtonFunc;
        typedef Func<void(float, float)>       OnCursorPosFunc;
        typedef Func<void(int)>                OnCursorEnterFunc;
        typedef Func<void(float, float)>       OnScrollFunc;
        typedef Func<void(int, const char**)>  OnDropFunc;
        typedef Func<void(uint32, uint32)>     OnWindowSizeFunc;
        typedef Func<void(bool)>               OnWindowFocus;
        void RegisterOnKeyFunc(OnKeyFunc&& func);
        void RegisterOnMouseButtonFunc(OnMouseButtonFunc&& func);
        void RegisterOnCursorPosFunc(OnCursorPosFunc&& func);
        void RegisterOnScrollFunc(OnScrollFunc&& func);
        void RegisterOnWindowSizeFunc(OnWindowSizeFunc&& func);
        void RegisterOnWindowFocusFunc(OnWindowFocus&& func);
        void RegisterOnDropFunc(OnDropFunc&& func);
	protected:
        TArray<OnKeyFunc>         m_OnKeyFunc;
        TArray<OnMouseButtonFunc> m_OnMouseButtonFunc;
        TArray<OnCursorPosFunc>   m_OnCursorPosFunc;
        TArray<OnScrollFunc>      m_OnScrollFunc;
        TArray<OnDropFunc>        m_OnDropFunc;
        TArray<OnWindowSizeFunc>  m_OnWindowSizeFunc;
        TArray<OnWindowFocus>     m_OnWindowFocusFunc;
	private:
        static TUniquePtr<EngineWindow> s_Instance;
	};

    // Map engine key code to system key code
    template<int NumStart, int NumEnd, typename Enum>
    class TInputBidirectionalMap {
    public:
        void AddPair(int num, Enum enm) {
            m_Num2Enum[num - NumStart] = enm;
            m_Enum2Num[EnumCast(enm)] = num;
        }
        int GetNum(Enum enm) { return m_Enum2Num[EnumCast(enm)]; }
        Enum GetEnum(int num) { return m_Num2Enum[num - NumStart]; }
    private:
        TStaticArray<Enum, NumEnd - NumStart + 1> m_Num2Enum;
        TStaticArray<int, EnumCast(Enum::Count)> m_Enum2Num;
    };

    template<typename Enum>
    class TInputBidirectionalMap2 {
    public:
        void AddPair(int num, Enum enm) {
            m_Num2Enum[num] = enm;
            m_Enum2Num[EnumCast(enm)] = num;
        }
        int GetNum(Enum enm) { return m_Enum2Num[EnumCast(enm)]; }
        Enum GetEnum(int num) { return m_Num2Enum[num]; }
    private:
        TUnorderedMap<int, Enum> m_Num2Enum;
        TStaticArray<int, EnumCast(Enum::Count)> m_Enum2Num;
    };
}