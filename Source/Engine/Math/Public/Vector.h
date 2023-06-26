#pragma once

namespace Math {
#define MATH_GENERIC template<typename T>

#define MATH_GENERIC_INSTANCE (x)\
    template struct x<float>;\
    template struct x<double>;\
    template struct x<int>

    // Vectors
    MATH_GENERIC struct Vector2 {
        T x{ 0.f };
    	T y{ 0.f };
        Vector2() = default;
        Vector2(T _x, T _y) : x(_x), y(_y) {}
        explicit Vector2(T scaler) : x(scaler), y(scaler) {}
        explicit Vector2(T* r) : x(r[0]), y(r[1]) {}

        T* Data() { return &x; }
        const T* Data() const { return &x; }

        T operator[](int i) const;
        T& operator[](int i);
        bool operator==(const Vector2<T>& rhs) const { return (x == rhs.x && y == rhs.y); }
        bool operator!=(const Vector2<T>& rhs) const { return (x != rhs.x || y != rhs.y); }
        // arithmetic operations
        Vector2<T> operator+(const Vector2<T>& rhs) const { return Vector2<T>(x + rhs.x, y + rhs.y); }
        Vector2<T> operator-(const Vector2<T>& rhs) const { return Vector2<T>(x - rhs.x, y - rhs.y); }
        Vector2<T> operator*(T scalar) const { return Vector2<T>(x * scalar, y * scalar); }
        Vector2<T> operator*(const Vector2<T>& rhs) const { return Vector2<T>(x * rhs.x, y * rhs.y); }
        Vector2<T> operator/(T scalar) const{ float inv = 1.0f / scalar; return Vector2<T>(x * inv, y * inv); }
        Vector2<T> operator/(const Vector2<T>& rhs) const { return Vector2<T>(x / rhs.x, y / rhs.y); }
        const Vector2<T>& operator+() const { return *this; }
        Vector2<T> operator-() const { return Vector2<T>(-x, -y); }
        // overloaded operators to help Vector2<T>
        friend Vector2<T> operator*(T scalar, const Vector2<T>& rhs) { return Vector2<T>(scalar * rhs.x, scalar * rhs.y); }
        friend Vector2<T> operator/(T scalar, const Vector2<T>& rhs) { return Vector2<T>(scalar / rhs.x, scalar / rhs.y); }
        friend Vector2<T> operator+(const Vector2<T>& lhs, T rhs) { return Vector2<T>(lhs.x + rhs, lhs.y + rhs); }
        friend Vector2<T> operator+(T lhs, const Vector2<T>& rhs) { return Vector2<T>(lhs + rhs.x, lhs + rhs.y); }
        friend Vector2<T> operator-(const Vector2<T>& lhs, T rhs) { return Vector2<T>(lhs.x - rhs, lhs.y - rhs); }
        friend Vector2<T> operator-(T lhs, const Vector2<T>& rhs) { return Vector2<T>(lhs - rhs.x, lhs - rhs.y); }

        // arithmetic updates
        Vector2<T>& operator+=(const Vector2<T>& rhs)
        {
            x += rhs.x;
            y += rhs.y;

            return *this;
        }

        Vector2<T>& operator+=(T scalar)
        {
            x += scalar;
            y += scalar;

            return *this;
        }

        Vector2<T>& operator-=(const Vector2<T>& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;

            return *this;
        }

        Vector2<T>& operator-=(T scalar)
        {
            x -= scalar;
            y -= scalar;

            return *this;
        }

        Vector2<T>& operator*=(T scalar)
        {
            x *= scalar;
            y *= scalar;

            return *this;
        }

        Vector2<T>& operator*=(const Vector2<T>& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;

            return *this;
        }

        Vector2<T>& operator/=(float scalar)
        {

            float inv = 1.0f / scalar;

            x *= inv;
            y *= inv;

            return *this;
        }

        Vector2<T>& operator/=(const Vector2<T>& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;

            return *this;
        }
        T Length() const;
        T SquaredLength() const { return x * x + y * y; }
        T Cross(const Vector2<T>& rhs) const { return x * rhs.y - y * rhs.x; }
        T Dot(const Vector2<T>& vec) const { return x * vec.x + y * vec.y; }
        T Distance(const Vector2<T>& rhs) const { return (*this - rhs).Length(); }
        T Normalize();
        Vector2<T> NormalizeCopy() { Vector2<T> r = { x, y }; r.Normalize(); return r; }

        static Vector2<T> Max(const Vector2<T>& v0, const Vector2<T>& v1);
        static Vector2<T> Min(const Vector2<T>& v0, const Vector2<T>& v1);

    };
     
    MATH_GENERIC struct Vector3 {
        T x{ 0.0f };
        T y{ 0.0f };
    	T z{ 0.0f };
        Vector3() = default;
        Vector3(T scalar) :x(scalar), y(scalar), z(scalar) {}
        Vector3(T _x, T _y, T _z) :x(_x), y(_y), z(_z) {}
        explicit Vector3(const T* coords) : x{ coords[0] }, y{ coords[1] }, z{ coords[2] } {}

        T* Data() { return &x; }
        const T* Data() const { return &x; }

        T operator[](int i) const;
        T& operator[](int i);
        bool operator==(const Vector3<T>& rhs) const { return (x == rhs.x && y == rhs.y && z == rhs.z); }
        bool operator!=(const Vector3<T>& rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z; }
        // arithmetic operations
        Vector3<T> operator+(const Vector3<T>& rhs) const { return Vector3<T>(x + rhs.x, y + rhs.y, z + rhs.z); }
        Vector3<T> operator-(const Vector3<T>& rhs) const { return Vector3<T>(x - rhs.x, y - rhs.y, z - rhs.z); }
        Vector3<T> operator*(T scalar) const { return Vector3<T>(x * scalar, y * scalar, z * scalar); }
        Vector3<T> operator*(const Vector3<T>& rhs) const { return Vector3<T>(x * rhs.x, y * rhs.y, z * rhs.z); }
        Vector3<T> operator/(T scalar) const;
        Vector3<T> operator/(const Vector3<T>& rhs) const;
        const Vector3<T>& operator+() const { return *this; }
        Vector3<T> operator-() const { return Vector3<T>(-x, -y, -z); }
        // overloaded operators to help Vector3<T>
        friend Vector3<T> operator*(T scalar, const Vector3<T>& rhs) { return Vector3<T>(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z); }
        friend Vector3<T> operator/(T scalar, const Vector3<T>& rhs);
        friend Vector3<T> operator+(const Vector3<T>& lhs, T rhs) { return Vector3<T>(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs); }
        friend Vector3<T> operator+(T lhs, const Vector3<T>& rhs) { return Vector3<T>(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z); }
        friend Vector3<T> operator-(const Vector3<T>& lhs, T rhs) { return Vector3<T>(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs); }
        friend Vector3<T> operator-(T lhs, const Vector3<T>& rhs) { return Vector3<T>(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z); }

        // arithmetic updates
        Vector3<T>& operator+=(const Vector3<T>& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }

        Vector3<T>& operator+=(float scalar)
        {
            x += scalar;
            y += scalar;
            z += scalar;
            return *this;
        }

        Vector3<T>& operator-=(const Vector3<T>& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }

        Vector3<T>& operator-=(float scalar)
        {
            x -= scalar;
            y -= scalar;
            z -= scalar;
            return *this;
        }

        Vector3<T>& operator*=(float scalar)
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        Vector3<T>& operator*=(const Vector3<T>& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            return *this;
        }

        Vector3<T>& operator/=(float scalar);
        Vector3<T>& operator/=(const Vector3<T>& rhs);
        T Length() const;
        T SquaredLength() const { return x * x + y * y + z * z; }
        T Distance(const Vector3<T>& rhs) const { return (*this - rhs).Length(); }
        T SquaredDistance(const Vector3<T>& rhs) const { return (*this - rhs).SquaredLength(); }
        T Dot(const Vector3<T>& vec) const { return x * vec.x + y * vec.y + z * vec.z; }
        void Normalize();
        Vector3<T> NormalizeCopy() const { Vector3<T> r{ x, y, z }; r.Normalize(); return r; }
        void Cross(const Vector3<T>& rhs) { x = y * rhs.z - z * rhs.y; y = z * rhs.x - x * rhs.z; z = x * rhs.y - y * rhs.x; }
        static T Dot(const Vector3<T>& v0, const Vector3<T>& v1) { return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z; }
        static Vector3<T> Cross(const Vector3<T>& v0, const Vector3<T>& v1) { return Vector3<T>(v0.y * v1.z - v0.z * v1.y, v0.z * v1.x - v0.x * v1.z, v0.x * v1.y - v0.y * v1.x); }
        static Vector3<T> Max(const Vector3<T>& v0, const Vector3<T>& v1);
        static Vector3<T> Min(const Vector3<T>& v0, const Vector3<T>& v1);
    };

    template<typename T>
    struct Vector4 {
        T x{ 0.f };
        T y{ 0.f };
        T z{ 0.f };
    	T w{ 0.f };

        Vector4() = default;
        Vector4(T _x, T _y, T _z, T _w) : x{ _x }, y{ _y }, z{ _z }, w{ _w } {}
        Vector4(const Vector3<T>& v3, T w_) : x{ v3.x }, y{ v3.y }, z{ v3.z }, w{ w_ } {}
        explicit Vector4(T* coords) : x{ coords[0] }, y{ coords[1] }, z{ coords[2] }, w{ coords[3] } {}

        T* Data() { return &x; }
        const T* Data() const { return &x; }

        T operator[](int i) const;
        T& operator[](int i);
        Vector4<T>& operator=(float scalar)
        {
            x = scalar;
            y = scalar;
            z = scalar;
            w = scalar;
            return *this;
        }
        bool operator==(const Vector4<T> & rhs) const { return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w); }
        bool operator!=(const Vector4<T> & rhs) const { return !(rhs == *this); }
        Vector4<T> operator+(const Vector4<T> & rhs) const { return Vector4<T>(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
        Vector4<T> operator-(const Vector4<T> & rhs) const { return Vector4<T>(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
        Vector4<T> operator*(T scalar) const { return Vector4<T>(x * scalar, y * scalar, z * scalar, w * scalar); }
        Vector4<T> operator*(const Vector4<T> & rhs) const { return Vector4<T>(rhs.x * x, rhs.y * y, rhs.z * z, rhs.w * w); }
        Vector4<T> operator/(T scalar) const;
        Vector4<T> operator/(const Vector4<T>& rhs) const;
        const Vector4<T>& operator+() const { return *this; }
        Vector4<T> operator-() const { return Vector4<T>(-x, -y, -z, -w); }
        friend Vector4<T> operator*(T scalar, const Vector4<T> & rhs) { return Vector4<T>(scalar * rhs.x, scalar * rhs.y, scalar * rhs.z, scalar * rhs.w); }
        friend Vector4<T> operator/(T scalar, const Vector4<T>& rhs);
        friend Vector4<T> operator+(const Vector4<T> & lhs, float rhs) { return Vector4<T>(lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs); }
        friend Vector4<T> operator+(T lhs, const Vector4<T> & rhs) { return Vector4<T>(lhs + rhs.x, lhs + rhs.y, lhs + rhs.z, lhs + rhs.w); }
        friend Vector4<T> operator-(const Vector4<T> & lhs, float rhs) { return Vector4<T>(lhs.x - rhs, lhs.y - rhs, lhs.z - rhs, lhs.w - rhs); }
        friend Vector4<T> operator-(T lhs, const Vector4<T> & rhs) { return Vector4<T>(lhs - rhs.x, lhs - rhs.y, lhs - rhs.z, lhs - rhs.w); }
        // arithmetic updates
        Vector4<T>& operator+=(const Vector4<T> & rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            w += rhs.w;
            return *this;
        }

        Vector4<T>& operator-=(const Vector4<T> & rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            w -= rhs.w;
            return *this;
        }

        Vector4<T>& operator*=(float scalar)
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            w *= scalar;
            return *this;
        }

        Vector4<T>& operator+=(float scalar)
        {
            x += scalar;
            y += scalar;
            z += scalar;
            w += scalar;
            return *this;
        }

        Vector4<T>& operator-=(float scalar)
        {
            x -= scalar;
            y -= scalar;
            z -= scalar;
            w -= scalar;
            return *this;
        }

        Vector4<T>& operator*=(const Vector4<T> & rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            w *= rhs.w;
            return *this;
        }

        Vector4<T>& operator/=(T scalar);
        Vector4<T>& operator/=(const Vector4<T>& rhs);
        T Dot(const Vector4<T> & vec) const { return x * vec.x + y * vec.y + z * vec.z + w * vec.w; }
        static Vector4<T> Max(const Vector4<T>& v0, const Vector4<T>& v1);
        static Vector4<T> Min(const Vector4<T>& v0, const Vector4<T>& v1);
    };

    typedef Vector2<float>         FVector2;
    typedef Vector2<double>        DVector2;
    typedef Vector2<int>           IVector2;
    typedef Vector2<unsigned int>  UVector2;
    typedef Vector2<unsigned char> UCVector2;

    typedef Vector3<float>         FVector3;
    typedef Vector3<double>        DVector3;
    typedef Vector3<int>	       IVector3;
    typedef Vector3<unsigned int>  UVector3;
    typedef Vector3<unsigned char> UCVector3;

    typedef Vector4<float>         FVector4;
    typedef Vector4<double>        DVector4;
    typedef Vector4<int>	       IVector4;
    typedef Vector4<unsigned int>  UVector4;
    typedef Vector4<unsigned char> UCVector4;   
}