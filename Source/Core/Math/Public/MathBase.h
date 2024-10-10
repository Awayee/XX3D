#pragma once
#include <cmath>

namespace Math {
	constexpr  float PI = 3.14159265358979323846264338327950288f;
	//const float Deg2Rad = PI / 180.0f;
	constexpr  float Deg2Rad = 0.017453292519943295f;
	//const float Rad2Deg = 180.0f / PI;
	constexpr  float Rad2Deg = 57.29577951308232f;
	template<typename T> T Min(T a, T b) { return a < b ? a : b; }
	template<typename T> T Max(T a, T b) { return a > b ? a : b; }
	template<typename T> T Abs(T x) { return x < (T)0 ? -x : x; }
	template<typename T> T Clamp(T x, T min, T max) { return x < max ? (x > min ? x : min) : max; }
	inline bool IsNaN(float f) { return std::isnan(f); };
	template<typename T> bool FloatEqual(float a, T b, float precision = 1e-6f) { return std::fabs(b - a) <= precision; }

	// rad is default
	inline float ToRad(float x) { return x * Deg2Rad; }
	inline float ToDeg(float x) { return x * Rad2Deg; }

	template<typename T> T Sqrt(T x) { return (T)std::sqrt(x); }
	template<typename T> T Hypot(T x, T y) { return (T)std::hypot(x, y); }
	template<typename T> T Hypot(T x, T y, T z) { return (T)std::hypot(x, y, z); }
	template<typename T> T Square(T x) { return x * x; }
	template<typename T> T Cube(T x) { return x * x * x; }
	template<typename T> T Pow(T x, T exp) { return (T)std::pow(x, exp); }
	template<typename T> T Ceil(T x) { return (T)std::ceil(x); }
	template<typename T> T Floor(T x) { return (T)std::floor(x); }

	// triangle func
#ifdef MATH_DEG
#define TRI_FUNC(func, f){ return func(f * Deg2Rad); }
#define ATRI_FUNC(func, f) { return func(f * Deg2Rad); }
#else
#define TRI_FUNC(func, f){ return func(f);}
#define ATRI_FUNC(func, ...) { return func( ##__VA_ARGS__); }
#endif
	template<typename T> T Sin   (T x)TRI_FUNC (std::sin , x);
	template<typename T> T Cos   (T x)TRI_FUNC (std::cos , x);
	template<typename T> T Tan   (T x)TRI_FUNC (std::tan , x);
	template<typename T> T ASin  (T x)ATRI_FUNC(std::asin, x);
	template<typename T> T ACos  (T x)ATRI_FUNC(std::acos, x);
	template<typename T> T ATan  (T x)ATRI_FUNC(std::atan, x);
	template<typename T> T ATan2 (T a, T b)ATRI_FUNC(std::atan2, a, b);


	// specialized
	inline float FMin(float a, float b) { return Min<float>(a, b); }
	inline float FMax(float a, float b) { return Max<float>(a, b); }
	inline float FAbs(float x) { return Abs<float>(x); }
	inline float FSqrt(float x) { return Sqrt<float>(x); }
	inline float FHypot(float x, float y) { return Hypot<float>(x, y); }
	inline float FHypot(float x, float y, float z) { return Hypot<float>(x, y, z); }
	inline float FPow(float x, float exp) { return Pow<float>(x, exp); }
	inline float FSin(float x) { return Sin<float>(x); }
	inline float FCos(float x) { return Cos<float>(x); }
	inline float FTan(float x) { return Tan<float>(x); }
	inline float FASin(float x) { return ASin<float>(x); }
	inline float FACos(float x) { return ACos<float>(x); }
	inline float FATan(float x) { return ATan<float>(x); }
	inline float FATan2(float x, float y) { return ATan2<float>(x, y); }

	//data pack
	inline unsigned char PackFloatS1(float x) { return (unsigned char)((x * 0.5f + 0.5f) * 255.0f); }
	inline float UnpackFloatS1(unsigned char x) { return (float)x / 255.0f * 2.0f - 1.0f; }
	inline unsigned char PackFloat01(float x) { return (unsigned char)(x * 255.0f); }
	inline float PackFloat01(unsigned char x) { return (float)x / 255.0f; }

	// reference https://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion
	// float32 to float16
	inline unsigned short FloatToHalf(float x) {
		// IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
		const unsigned int b = *(const unsigned int*)(&x) + 0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
		const unsigned int e = (b & 0x7F800000) >> 23; // exponent
		const unsigned int m = b & 0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
		return (b & 0x80000000) >> 16 | (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) | ((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) | (e > 143) * 0x7FFF; // sign : normalized : denormalized : saturate
	}
	// float16 to float32
	inline float HalfToFloat(unsigned short x) {
		// IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
		const unsigned int e = (x & 0x7C00) >> 10; // exponent
		const unsigned int m = (x & 0x03FF) << 13; // mantissa
		const float mFloat = (float)m;
		const unsigned int mUint = *(const unsigned int*)(&mFloat);
		const unsigned int v = mUint >> 23; // evil log2 bit hack to count leading zeros in denormalized format
		const unsigned int result = (x & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) | ((e == 0) & (m != 0)) * ((v - 37) << 23 | ((m << (150 - v)) & 0x007FE000)); // sign : normalized : denormalized
		return *(const float*)&result;
	}

	template<typename T> T UpperExp2(T x) {
		if (!(x & (x - 1))) {
			return x;
		}
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return x + 1;
	}

	template<typename T> T LowerExp2(T x) {
		if (!(x & (x - 1))) {
			return x;
		}
		return UpperExp2<T>(x >> 1);
	}

	template<typename T> bool IsNearlyZero(T val) { return val == (T)0; }

	template<> inline bool IsNearlyZero(float val) {
		constexpr float e = 1e-6f;
		return Abs(val) < e;
	}

	template<> inline bool IsNearlyZero(double val) {
		constexpr double e = 1e-6;
		return Abs(val) < e;
	}
}