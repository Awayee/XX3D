#include "Math/Public/Quaternion.h"
#include "Math/Public/MathBase.h"
#include <glm/gtc/quaternion.hpp>
namespace Math {
    template <typename T> Quaternion<T> Quaternion<T>::AngleAxis(T a, const Vector3<T>& axis){
        Quaternion<T> q;
        T hanfA = (T)0.5f * a;
        T sinV = Math::Sin<T>(hanfA);
        q.w = Math::Cos<T>(hanfA);
        q.x = axis.x * sinV;
        q.y = axis.y * sinV;
        q.z = axis.z * sinV;
        return q;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::Euler(const Vector3<T>& euler) {
        T cosR = Math::Cos(euler.x * 0.5);//roll
        T sinR = Math::Sin(euler.x * 0.5);
        T cosP = Math::Cos(euler.y * 0.5);//pitch
        T sinP = Math::Sin(euler.y * 0.5);
		T cosY = Math::Cos(euler.z * 0.5);//yaw
		T sinY = Math::Sin(euler.z * 0.5);
		return {
			sinP * sinY * cosR + cosP * cosY * sinR,
			sinP * cosY * cosR - cosP * sinY * sinR,
			cosP * sinY * cosR - sinP * cosY * sinR,
			cosP * cosY * cosR + sinP * sinY * sinR
		};
    }

    template <typename T> T Quaternion<T>::Roll() const {
        return static_cast<T>(Math::ATan2(static_cast<T>(2) * (x * y + w * z), w * w + x * x - y * y - z * z));
    }

    template <typename T> T Quaternion<T>::Pitch() const {
        return static_cast<T>(Math::ATan2(static_cast<T>(2) * (y * z + w * x), w * w - x * x - y * y + z * z));
    }

    template <typename T> T Quaternion<T>::Yaw() const {
        return Math::ASin<T>(Math::Clamp<T>(static_cast<T>(-2) * (x * z - w * y), static_cast<T>(-1), static_cast<T>(1)));
    }

    template <typename T>
    Vector3<T> Quaternion<T>::ToEuler() const {
        return { Pitch(), Yaw(), Roll() };
    }

    template <typename T>
    Vector3<T> Quaternion<T>::RotateVector3(const Vector3<T>& v) const{
        // nVidia SDK implementation
        Vector3<T> uv, uuv;
        Vector3<T> qvec(x, y, z);
        uv   = Vector3<T>::Cross(qvec, v);
        uuv  = Vector3<T>::Cross(qvec, uv);
        uv  *=  (2.0f * w);
        uuv *=  2.0f;

        return v + uv + uuv;
    }

    template <typename T>
    Quaternion<T> Quaternion<T>::Inverse() const{
        T norm = w * w + x * x + y * y + z * z;
        if (norm > 0.0)
        {
            T inv_norm = 1.0f / norm;
            return Quaternion<T>(w * inv_norm, -x * inv_norm, -y * inv_norm, -z * inv_norm);
        }
        else
        {
            // return an invalid result to flag the error
            return Quaternion<T>{};
        }
    }

    template <typename T> Quaternion<T> Quaternion<T>::operator*(const Quaternion<T>& q) const{
        return Quaternion<T>(
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y + y * q.w + z * q.x - x * q.z,
            w * q.z + z * q.w + x * q.y - y * q.x,
            w * q.w - x * q.x - y * q.y - z * q.z);
    }

    template <typename T> Quaternion<T> Quaternion<T>::operator*= (const Quaternion<T>& q){
        x = w * q.x + x * q.w + y * q.z - z * q.y;
        y = w * q.z + z * q.w + x * q.y - y * q.x;
        z = w * q.z + z * q.w + x * q.y - y * q.x;
        w = w * q.w - x * q.x - y * q.y - z * q.z;
        return *this;
    }

    /*
    template <typename T> Matrix4x4<T> Quaternion<T>::ToRotateMatrix() const
    {
        float fTx = x + x;   // 2x
        float fTy = y + y;   // 2y
        float fTz = z + z;   // 2z
        float fTwx = fTx * w; // 2xw
        float fTwy = fTy * w; // 2yw
        float fTwz = fTz * w; // 2z2
        float fTxx = fTx * x; // 2x^2
        float fTxy = fTy * x; // 2xy
        float fTxz = fTz * x; // 2xz
        float fTyy = fTy * y; // 2y^2
        float fTyz = fTz * y; // 2yz
        float fTzz = fTz * z; // 2z^2
        return {
            (T)1 - (fTyy + fTzz), fTxy - fTwz, fTxz + fTwy, 0,
            fTxy + fTwz, (T)1 - (fTxx + fTzz), fTyz - fTwx, 0,
            fTxz - fTwy, fTyz + fTwx, (T)1 - (fTxx + fTyy), 0,
            0, 0, 0, 1
        };
    }
    */
    template <typename T> const Quaternion<T> Quaternion<T>::ZERO(0,0,0,0);
    template <typename T> const Quaternion<T> Quaternion<T>::IDENTITY(0,0,0,1);

    template Quaternion<float>;
    template Quaternion<double>;
}