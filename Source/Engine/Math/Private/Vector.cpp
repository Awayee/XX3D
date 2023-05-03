#include "Math/Public/MathBase.h"
#include "Math/Public/Vector.h"
#include "Core/Public/macro.h"

namespace Math {
    // vector2
    MATH_GENERIC T Vector2<T>::operator[](int i) const
    {
        ASSERT(i < 2);
        return (i == 0 ? x : y);
    }

    MATH_GENERIC T& Vector2<T>::operator[](int i)
    {
        ASSERT(i < 2);
        return (i == 0 ? x : y);
    }

    MATH_GENERIC T Vector2<T>::Length() const
    {
        return Hypot<T>(x, y);
    }

    MATH_GENERIC T Vector2<T>::Normalize()
    {
        T lengh = Hypot<T>(x, y);

        if (lengh > 0)
        {
            T inv_length = (T)1 / lengh;
            x *= inv_length;
            y *= inv_length;
        }

        return lengh;
    }
    MATH_GENERIC Vector2<T> Vector2<T>::Max(const Vector2<T>& v0, const Vector2<T>& v1)
    {
        return { Math::Max<T>(v0.x, v1.x), Math::Max<T>(v0.y, v1.y) };
    }
    MATH_GENERIC Vector2<T> Vector2<T>::Min(const Vector2<T>& v0, const Vector2<T>& v1)
    {
        return { Math::Min<T>(v0.x, v1.x), Math::Min<T>(v0.y, v1.y) };
    }

    // vector3
    MATH_GENERIC T Vector3<T>::operator[](int i) const
    {
        ASSERT(i < 3);
        return *(&x + i);
    }
    MATH_GENERIC T& Vector3<T>::operator[](int i)
    {
        ASSERT(i < 3);
        return *(&x + i);
    }
    MATH_GENERIC Vector3<T> Vector3<T>::operator/(T scalar) const
    {
        ASSERT(scalar != 0.0);
        return Vector3<T>(x / scalar, y / scalar, z / scalar);
    }
    MATH_GENERIC Vector3<T> Vector3<T>::operator/(const Vector3<T>& rhs) const
    {
        ASSERT((rhs.x != 0 && rhs.y != 0 && rhs.z != 0));
        return Vector3<T>(x / rhs.x, y / rhs.y, z / rhs.z);
    }
    MATH_GENERIC Vector3<T>& Vector3<T>::operator/=(float scalar)
    {
        ASSERT(scalar != 0.0);
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }
    MATH_GENERIC Vector3<T>& Vector3<T>::operator/=(const Vector3<T>& rhs)
    {
        ASSERT(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        return *this;
    }
    MATH_GENERIC Vector3<T> operator/(T scalar, const Vector3<T>& rhs)
    {
        ASSERT(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
        return Vector3(scalar / rhs.x, scalar / rhs.y, scalar / rhs.z);
    }
    MATH_GENERIC T Vector3<T>::Length() const
    {
        return Hypot<T>(x, y, z);
    }
    MATH_GENERIC void Vector3<T>::Normalize()
    {
        T length = Hypot<T>(x, y, z);
        if (length == 0)
            return;

        T lengthInv = (T)1/ length;
        x *= lengthInv;
        y *= lengthInv;
        z *= lengthInv;
    }
    MATH_GENERIC Vector3<T> Vector3<T>::Max(const Vector3<T>& v0, const Vector3<T>& v1)
    {
        return { Math::Max<T>(v0.x, v1.x), Math::Max<T>(v0.y, v1.y), Math::Max<T>(v0.z, v1.z) };
    }
    MATH_GENERIC Vector3<T> Vector3<T>::Min(const Vector3<T>& v0, const Vector3<T>& v1)
    {
        return { Math::Min<T>(v0.x, v1.x), Math::Min<T>(v0.y, v1.y), Math::Min<T>(v0.z, v1.z) };
    }

    // vector4
    MATH_GENERIC T Vector4<T>::operator[](int i) const
    {
        ASSERT(i < 4);
        return *(&x + i);
    }
    MATH_GENERIC T& Vector4<T>::operator[](int i)
    {
        ASSERT(i < 4);
        return *(&x + i);
    }
    MATH_GENERIC Vector4<T> Vector4<T>::operator/(T scalar) const
    {
        ASSERT(scalar != 0.0);
        return Vector4<T>(x / scalar, y / scalar, z / scalar, w / scalar);
    }
    MATH_GENERIC Vector4<T> Vector4<T>::operator/(const Vector4<T>& rhs) const
    {
        ASSERT(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
        return Vector4<T>(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
    }
    MATH_GENERIC Vector4<T> operator/(T scalar, const Vector4<T>& rhs)
    {
        ASSERT(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
        return Vector4(scalar / rhs.x, scalar / rhs.y, scalar / rhs.z, scalar / rhs.w);
    }
    MATH_GENERIC Vector4<T>& Vector4<T>::operator/=(T scalar)
    {
        ASSERT(scalar != 0.0);
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }
    MATH_GENERIC Vector4<T>& Vector4<T>::operator/=(const Vector4<T>& rhs)
    {
        ASSERT(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        w /= rhs.w;
        return *this;
    }
    MATH_GENERIC Vector4<T> Vector4<T>::Max(const Vector4<T>& v0, const Vector4<T>& v1)
    {
        return { Math::Max<T>(v0.x, v1.x), Math::Max<T>(v0.y, v1.y), Math::Max<T>(v0.z, v1.z), Math::Max<T>(v0.w, v1.w)};
    }
    MATH_GENERIC Vector4<T> Vector4<T>::Min(const Vector4<T>& v0, const Vector4<T>& v1)
    {
        return { Math::Min<T>(v0.x, v1.x), Math::Min<T>(v0.y, v1.y), Math::Min<T>(v0.z, v1.z), Math::Min<T>(v0.w, v1.w)};
    }

    template Vector2<float>;
    template Vector2<double>;
    template Vector2<int>;
    template Vector2<unsigned int>;
    template Vector2<unsigned char>;

    template Vector3<float>;
    template Vector3<double>;
    template Vector3<int>;
    template Vector3<unsigned int>;
    template Vector3<unsigned char>;

    template Vector4<float>;
    template Vector4<double>;
    template Vector4<int>;
    template Vector4<unsigned int>;
    template Vector4<unsigned char>;
}