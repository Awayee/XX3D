#pragma once
#include "TypeDefine.h"

template<typename T> struct Size2D {
	T w{ 0 }, h{ 0 };
};
typedef Size2D<uint32> USize2D;
typedef Size2D<int32>  ISize2D;
typedef Size2D<float>  FSize2D;

template<typename T> struct Size3D {
	T w{ 0 }, h{ 0 }, d{ 0 };
};
typedef Size3D<uint32> USize3D;
typedef Size3D<float> FSize3D;

template<typename T> struct Offset2D {
	T x{ 0 }, y{ 0 };
};
typedef Offset2D<float>  FOffset2D;
typedef Offset2D<uint32> UOffset2D;

template<typename T> struct Offset3D {
	T x{ 0 }, y{ 0 }, z{ 0 };
};
typedef Offset3D<uint32> UOffset3D;
typedef Offset3D<float>  FOffset3D;

template<typename TPos, typename TSize> struct Rect {
	TPos x{ 0 }, y{ 0 };
	TSize w{ 0 }, h{ 0 };
};
typedef Rect<float, float> FRect;
typedef Rect<int32, int32> IRect;
typedef Rect<uint32, uint32> URect;


template<typename T> struct Color3 {
	T r, g, b;
};

typedef Color3<uint8> U8Color3;
typedef Color3<float> FColor3;

template<typename T> struct Color4 {
	T r, g, b, a;
};
typedef Color4<uint8> U8Color4;
typedef Color4<float> FColor4;