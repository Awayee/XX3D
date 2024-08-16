#pragma once

typedef char				int8;
typedef unsigned char		uint8;
typedef short				int16;
typedef unsigned short		uint16;
typedef int					int32;
typedef unsigned int		uint32;
typedef long long			int64;
typedef unsigned long long	uint64;


// These macros must exactly match those in the Windows SDK's intsafe.h.
#define INT8_MIN         (-127i8 - 1)
#define INT16_MIN        (-32767i16 - 1)
#define INT32_MIN        (-2147483647i32 - 1)
#define INT64_MIN        (-9223372036854775807i64 - 1)
#define INT8_MAX         127i8
#define INT16_MAX        32767i16
#define INT32_MAX        2147483647i32
#define INT64_MAX        9223372036854775807i64
#define UINT8_MAX        0xffui8
#define UINT16_MAX       0xffffui16
#define UINT32_MAX       0xffffffffui32
#define UINT64_MAX       0xffffffffffffffffui64

#define FLOAT_MAX        3.402823466e+38f
#define FLOAT_MIN        (-FLOAT_MAX)

#define XX_NODISCARD [[nodiscard]]

#define MoveTemp(x) (std::move(x))


#define NON_COPYABLE(cls)\
	cls(const cls&) = delete;\
	cls& operator=(const cls&) = delete

#define NON_MOVEABLE(cls)\
	cls(cls&&) noexcept = delete; \
	cls& operator=(cls&&)noexcept = delete

#define SINGLETON_INSTANCE(cls)\
public:\
	static void Release(){ if(s_Instance) delete s_Instance; }\
	template <class ...Args> static void Initialize(Args...args) { Release(); s_Instance = new cls(args...); }\
	static cls* Instance() {return s_Instance; }\
private:\
	NON_COPYABLE(cls);\
	NON_MOVEABLE(cls);\
	inline static cls* s_Instance {nullptr}