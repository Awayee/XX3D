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

#define INT_CAST(val)   static_cast<int>(val)
#define UINT8_CAST(val) static_cast<uint8>((val) > 0 ? (val) : 0)
#define UINT16_CAST(val) static_cast<uint16>((val) > 0 ? (val) : 0)
#define UINT32_CAST(val) static_cast<uint32>((val) > 0 ? (val) : 0)
#define UINT64_CAST(val) static_cast<uint64>((val) > 0 ? (val) : 0)
	