#include "Math/Public/Quaternion.h"
#include "Math/Public/MathBase.h"
namespace Math {
    MATH_GENERIC Quaternion<T> Quaternion<T>::AngleAxis(T a, const Vector3<T>& axis){
        Quaternion<T> q;
        T hanfA = (T)0.5f * a;
        T sinV = Math::Sin<T>(hanfA);
        q.w = Math::Cos<T>(hanfA);
        q.x = axis.x * sinV;
        q.y = axis.y * sinV;
        q.z = axis.z * sinV;
        return q;
    }
    MATH_GENERIC Vector3<T> Quaternion<T>::RotateVector3(const Vector3<T>& v) const{
        // nVidia SDK implementation
        Vector3<T> uv, uuv;
        Vector3<T> qvec(x, y, z);
        uv   = Vector3<T>::Cross(qvec, v);
        uuv  = Vector3<T>::Cross(qvec, uv);
        uv  *=  (2.0f * w);
        uuv *=  2.0f;

        return v + uv + uuv;
    }
    MATH_GENERIC Quaternion<T> Quaternion<T>::Inverse() const{
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

    MATH_GENERIC Quaternion<T> Quaternion<T>::operator*(const Quaternion<T>& q) const{
        return Quaternion<T>(
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y + y * q.w + z * q.x - x * q.z,
            w * q.z + z * q.w + x * q.y - y * q.x,
            w * q.w - x * q.x - y * q.y - z * q.z);
    }

    MATH_GENERIC Quaternion<T> Quaternion<T>::operator*= (const Quaternion<T>& q){
        x = w * q.x + x * q.w + y * q.z - z * q.y;
        y = w * q.z + z * q.w + x * q.y - y * q.x;
        z = w * q.z + z * q.w + x * q.y - y * q.x;
        w = w * q.w - x * q.x - y * q.y - z * q.z;
        return *this;
    }

    /*
    MATH_GENERIC Matrix4x4<T> Quaternion<T>::ToRotateMatrix() const
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
    MATH_GENERIC const Quaternion<T> Quaternion<T>::ZERO(0,0,0,0);
    MATH_GENERIC const Quaternion<T> Quaternion<T>::IDENTITY(0,0,0,1);

    template Quaternion<float>;
    template Quaternion<double>;
}