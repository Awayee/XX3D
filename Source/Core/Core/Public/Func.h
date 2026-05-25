#pragma once
#include <functional>
#include <type_traits>
#include <cstddef>
#include "Core/Public/Defines.h"

template<typename T>
using Func = std::function<T>;

template<typename Ret, typename... Args>
class XFuncBase {
public:
    virtual ~XFuncBase() = default;
    virtual Ret operator()(Args...args) = 0;
};

static constexpr uint32 PTR_SIZE = sizeof(void*);
static constexpr uint32 MAX_FUNC_STORAGE_SIZE = 2 * PTR_SIZE; // Store data on heap if size of struct larger than this.

template<typename Cls, typename Ret, typename...Args>
class XFuncObject {
public:
    typedef Ret(Cls::* MemFuncPtr)(Args...);
	XFuncObject(Cls* obj, MemFuncPtr func): m_Object(obj), m_FuncPtr(func) {}
    Ret operator()(Args...args) {
        return (m_Object->*m_FuncPtr)(std::forward<Args>(args)...);
	}
private:
    Cls* m_Object;
    MemFuncPtr m_FuncPtr;
};

template<typename Ret, typename... Args>
struct XCallerBase {
    virtual ~XCallerBase() = default;
    virtual Ret Invoke(void* data, Args...args) = 0;
    virtual void Destruct(void* data) = 0;
};

template<typename Ret, typename... Args>
struct XCallerStatic : XCallerBase<Ret, Args...> {
    using FuncType = Ret(*)(Args...);
    virtual Ret Invoke(void* data, Args...args)override {
        return (*(FuncType*)data)(std::forward<Args>(args)...);
    }
    virtual void Destruct(void* data) override {
    }
};

template<typename Cls, typename Ret, typename... Args>
struct XCallerObject : XCallerBase<Ret, Args...> {
    using FuncType = XFuncObject<Cls, Ret, Args...>;
    virtual Ret Invoke(void* data, Args...args)override {
        return ((FuncType*)data)->operator()(std::forward<Args>(args)...);
    }
    virtual void Destruct(void* data) override {
        ((FuncType*)data)->~XObjectFunc();
    }
};

template<typename Lambda, typename Ret, typename...Args>
struct XCallerLambda : XCallerBase<Ret, Args...> {
    virtual Ret Invoke(void* data, Args...args) override {
        Lambda* lambdaPtr;
        if constexpr (sizeof(Lambda) > MAX_FUNC_STORAGE_SIZE) {
            lambdaPtr = *(Lambda**)data;
        }
        else {
            lambdaPtr = (Lambda*)data;
        }
        return (*lambdaPtr)(std::forward<Args>(args)...);
    }
    virtual void Destruct(void* data) override {
        if constexpr (sizeof(Lambda) > MAX_FUNC_STORAGE_SIZE) {
            delete *(Lambda**)data;
        }
        else {
            ((Lambda*)data)->~Lambda();
        }
    }
};

// Common function type
template<typename Ret, typename...Args>
class XFunc {
    using Caller = XCallerBase<Ret, Args...>;
public:
    XFunc() {
        Reset();
    }
    // Static function
    XFunc(Ret(*staticFunc)(Args...)) {
        new (m_Caller.Data) XCallerStatic<Ret, Args...>();
        (*(void**)m_Data) = (void*)staticFunc;
    }
    // Class member
    template<typename Cls>
    XFunc(Cls* obj, Ret(Cls::* memFunc)(Args...)) {
        new (m_Caller.Data) XCallerObject<Cls, Ret, Args>();
        new (m_Data) XFuncObject<Cls, Ret, Args...>(obj, memFunc);
    }
    // lambda
    template<typename Lambda>
    XFunc(Lambda&& lambda) {
        using DecayF = std::decay_t<Lambda>;
        new(m_Caller.Data) XCallerLambda<DecayF, Ret, Args...>();
        if(sizeof(Lambda) > MAX_FUNC_STORAGE_SIZE) {
            (*(DecayF**)m_Data)= new DecayF(std::forward<Lambda>(lambda));
        }
        else {
            new (m_Data) DecayF(std::forward<Lambda>(lambda));
        }
    }

    XFunc(const XFunc&) = delete;
    XFunc(XFunc&& rhs)noexcept {
        memcpy(m_Caller.Data, rhs.m_Caller.Data, PTR_SIZE);
        memcpy(m_Data, rhs.m_Data, MAX_FUNC_STORAGE_SIZE);
        rhs.Reset();
    }
    ~XFunc() {
        Destroy();
    }
    XFunc& operator=(const XFunc&) = delete;
    XFunc& operator=(XFunc&& rhs) noexcept {
        Destroy();
        memcpy(m_Caller.Data, rhs.m_Caller.Data, PTR_SIZE);
        memcpy(m_Data, rhs.m_Data, MAX_FUNC_STORAGE_SIZE);
        rhs.Reset();
        return *this;
    }

    Ret operator()(Args...args) {
        return ((Caller*)m_Caller.Data)->Invoke(m_Data, std::forward<Args>(args)...);
    }

    operator bool() const {
        return !!m_Caller.Value;
    }

private:
    union {
        char Data[PTR_SIZE];
        void* Value; // For getting value, don't use this
    }m_Caller;
    char m_Data[MAX_FUNC_STORAGE_SIZE];

    inline void Reset() {
        memset(m_Caller.Data, 0, PTR_SIZE);
        memset(m_Data, 0, MAX_FUNC_STORAGE_SIZE);
    }

    inline void Destroy() {
        if(m_Caller.Value) {
            ((Caller*)m_Caller.Data)->Destruct(m_Data);
            ((Caller*)m_Caller.Data)->~XCallerBase();
            Reset();
        }
    }
};

// Sample usage
#if false
void TestXFunc() {
    XFunc<float, float> f0(std::cos);
    f0(0.1f);
    XFunc<float, float> f1([](float n) {return m * 2.0f; });
    f1(0.1f);
}
#endif