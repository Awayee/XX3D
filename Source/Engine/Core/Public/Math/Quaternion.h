#pragma once
#include "Vector.h"
namespace Math {
	MATH_GENERIC class Quaternion {
	public:
		T x{ 0.0f };
		T y{ 0.0f };
		T z{ 0.0f };
		T w{ 1.0f };
		Quaternion() = default;
		Quaternion(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}

		T* Data() { return &x; }
		const T* Data() const { return &x; }

        static Quaternion<T> AngleAxis(T a, const Vector3<T>& axis);
		static Quaternion<T> Euler(const Vector3<T>& euler);//z->x->y
		T Roll()const;
		T Pitch() const;
		T Yaw() const;
		Vector3<T> ToEuler() const;
        Vector3<T> RotateVector3(const Vector3<T>& v) const;
        Quaternion<T> Inverse() const; // apply to non-zero quaternion
		Quaternion<T> operator*(const Quaternion<T>& q) const;
		Quaternion<T> operator*=(const Quaternion<T>& q);
		static const Quaternion<T> ZERO;
		static const Quaternion<T> IDENTITY;
	};

	typedef Quaternion<float>  FQuaternion;
	typedef Quaternion<double> DQuaternion;
}
