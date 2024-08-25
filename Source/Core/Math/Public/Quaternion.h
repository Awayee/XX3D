#pragma once
#include "Vector.h"
namespace Math {
	template<typename T> class Quaternion {
	public:
		T X{ 0.0f };
		T Y{ 0.0f };
		T Z{ 0.0f };
		T W{ 1.0f };
		Quaternion() = default;
		Quaternion(T _x, T _y, T _z, T _w) : X(_x), Y(_y), Z(_z), W(_w) {}

		T* Data() { return &X; }
		const T* Data() const { return &X; }

        static Quaternion<T> AngleAxis(T a, const Vector3<T>& axis);
		static Quaternion<T> Euler(const Vector3<T>& euler);//z->x->y
		static Quaternion<T> FromTo(const Vector3<T>& from, const Vector3<T>& to);
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
