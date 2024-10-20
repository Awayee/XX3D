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

		T* Data() {
			return &X;
		}
		const T* Data() const {
			return &X;
		}

        static Quaternion<T> AngleAxis(T a, const Vector3<T>& axis) {
			Quaternion<T> q;
			T hanfA = (T)0.5f * a;
			T sinV = Math::Sin<T>(hanfA);
			q.W = Math::Cos<T>(hanfA);
			q.X = axis.X * sinV;
			q.Y = axis.Y * sinV;
			q.Z = axis.Z * sinV;
			return q;
		}
		static Quaternion<T> Euler(const Vector3<T>& euler) {
			//z->x->y
			T cosR = Math::Cos<T>(euler.X * (T)0.5);//roll
			T sinR = Math::Sin<T>(euler.X * (T)0.5);
			T cosP = Math::Cos<T>(euler.Y * (T)0.5);//pitch
			T sinP = Math::Sin<T>(euler.Y * (T)0.5);
			T cosY = Math::Cos<T>(euler.Z * (T)0.5);//yaw
			T sinY = Math::Sin<T>(euler.Z * (T)0.5);
			return {
				sinP * sinY * cosR + cosP * cosY * sinR,
				sinP * cosY * cosR - cosP * sinY * sinR,
				cosP * sinY * cosR - sinP * cosY * sinR,
				cosP * cosY * cosR + sinP * sinY * sinR
			};
		}
		static Quaternion<T> FromTo(const Vector3<T>& from, const Vector3<T>& to) {
			auto axis = from.Cross(to);
			T cosAngle = from.Dot(to);
			T angle = Math::ACos(cosAngle);
			return AngleAxis(angle, axis);
		}
		T Roll()const {
			return static_cast<T>(Math::ATan2(static_cast<T>(2) * (X * Y + W * Z), W * W + X * X - Y * Y - Z * Z));
		}
		T Pitch() const {
			return static_cast<T>(Math::ATan2(static_cast<T>(2) * (Y * Z + W * X), W * W - X * X - Y * Y + Z * Z));
		}
		T Yaw() const {
			return Math::ASin<T>(Math::Clamp<T>(static_cast<T>(-2) * (X * Z - W * Y), static_cast<T>(-1), static_cast<T>(1)));
		}
		Vector3<T> ToEuler() const {
			return { Pitch(), Yaw(), Roll() };
		}
        Vector3<T> RotateVector3(const Vector3<T>& v) const {
			// nVidia SDK implementation
			Vector3<T> uv, uuv;
			Vector3<T> qvec(X, Y, Z);
			uv = Vector3<T>::Cross(qvec, v);
			uuv = Vector3<T>::Cross(qvec, uv);
			uv *= (2.0f * W);
			uuv *= 2.0f;

			return v + uv + uuv;
		}
		Quaternion<T> Inverse() const {
			// apply to non-zero quaternion
			T norm = W * W + X * X + Y * Y + Z * Z;
			if (norm > 0.0){
				T inv_norm = 1.0f / norm;
				return Quaternion<T>(W * inv_norm, -X * inv_norm, -Y * inv_norm, -Z * inv_norm);
			}
			else{
				// return an invalid result to flag the error
				return Quaternion<T>{};
			}
		}
		Quaternion<T> operator*(const Quaternion<T>& q) const {
			return Quaternion<T>(
				W * q.X + X * q.W + Y * q.Z - Z * q.Y,
				W * q.Y + Y * q.W + Z * q.X - X * q.Z,
				W * q.Z + Z * q.W + X * q.Y - Y * q.X,
				W * q.W - X * q.X - Y * q.Y - Z * q.Z);
		}
		Quaternion<T> operator*=(const Quaternion<T>& q) {
			X = W * q.X + X * q.W + Y * q.Z - Z * q.Y;
			Y = W * q.Z + Z * q.W + X * q.Y - Y * q.X;
			Z = W * q.Z + Z * q.W + X * q.Y - Y * q.X;
			W = W * q.W - X * q.X - Y * q.Y - Z * q.Z;
			return *this;
		}
		static const Quaternion<T> IDENTITY;
	};

	template<typename T> const Quaternion<T> Quaternion<T>::IDENTITY{ 0,0,0,1 };

	typedef Quaternion<float>  FQuaternion;
	typedef Quaternion<double> DQuaternion;
}
