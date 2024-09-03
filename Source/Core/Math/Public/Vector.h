#pragma once
#include "MathBase.h"
namespace Math {

    // Vectors
    template <typename T> struct Vector2 {
        T X{ 0.f };
    	T Y{ 0.f };
        static const Vector2<T> ZERO;
        Vector2() = default;
        Vector2(T _x, T _y) : X(_x), Y(_y) {}
        explicit Vector2(T scaler) : X(scaler), Y(scaler) {}
        explicit Vector2(const T* r) : X(r[0]), Y(r[1]) {}

        T* Data() { return &X; }
        const T* Data() const { return &X; }

        T operator[](int i) const { return (i == 0 ? X : Y); }
        T& operator[](int i) { return (i == 0 ? X : Y); }
        bool operator==(const Vector2<T>& rhs) const {
	        return (X == rhs.X && Y == rhs.Y);
        }
        bool operator!=(const Vector2<T>& rhs) const {
	        return (X != rhs.X || Y != rhs.Y);
        }
        // arithmetic operations
        Vector2<T> operator+(const Vector2<T>& rhs) const {
	        return Vector2<T>(X + rhs.X, Y + rhs.Y);
        }
        Vector2<T> operator-(const Vector2<T>& rhs) const {
	        return Vector2<T>(X - rhs.X, Y - rhs.Y);
        }
        Vector2<T> operator*(T scalar) const {
	        return Vector2<T>(X * scalar, Y * scalar);
        }
        Vector2<T> operator*(const Vector2<T>& rhs) const {
	        return Vector2<T>(X * rhs.X, Y * rhs.Y);
        }
        Vector2<T> operator/(T scalar) const {
	        T inv = (T)1 / scalar;
        	return Vector2<T>(X * inv, Y * inv);
        }
        Vector2<T> operator/(const Vector2<T>& rhs) const {
	        return Vector2<T>(X / rhs.X, Y / rhs.Y);
        }
        // overloaded operators to help Vector2<T>
        friend Vector2<T> operator*(T scalar, const Vector2<T>& rhs) {
	        return Vector2<T>(scalar * rhs.X, scalar * rhs.Y);
        }
        friend Vector2<T> operator/(T scalar, const Vector2<T>& rhs) {
	        return Vector2<T>(scalar / rhs.X, scalar / rhs.Y);
        }
        friend Vector2<T> operator+(const Vector2<T>& lhs, T rhs) {
	        return Vector2<T>(lhs.X + rhs, lhs.Y + rhs);
        }
        friend Vector2<T> operator+(T lhs, const Vector2<T>& rhs) {
	        return Vector2<T>(lhs + rhs.X, lhs + rhs.Y);
        }
        friend Vector2<T> operator-(const Vector2<T>& lhs, T rhs) {
	        return Vector2<T>(lhs.X - rhs, lhs.Y - rhs);
        }
        friend Vector2<T> operator-(T lhs, const Vector2<T>& rhs) {
	        return Vector2<T>(lhs - rhs.X, lhs - rhs.Y);
        }
        // arithmetic updates
        Vector2<T>& operator+=(const Vector2<T>& rhs) {
            X += rhs.X;
            Y += rhs.Y;
            return *this;
        }
        Vector2<T>& operator+=(T scalar) {
            X += scalar;
            Y += scalar;
            return *this;
        }
        Vector2<T>& operator-=(const Vector2<T>& rhs) {
            X -= rhs.X;
            Y -= rhs.Y;
            return *this;
        }
        Vector2<T>& operator-=(T scalar) {
            X -= scalar;
            Y -= scalar;
            return *this;
        }
        Vector2<T>& operator*=(T scalar) {
            X *= scalar;
            Y *= scalar;
            return *this;
        }
        Vector2<T>& operator*=(const Vector2<T>& rhs) {
            X *= rhs.X;
            Y *= rhs.Y;
            return *this;
        };
        Vector2<T>& operator/=(T scalar) {
            T inv = (T)1 / scalar;
            X *= inv;
            Y *= inv;
            return *this;
        }
        Vector2<T>& operator/=(const Vector2<T>& rhs) {
            X /= rhs.X;
            Y /= rhs.Y;
            return *this;
        }
        T Length() const {
	        return Hypot<T>(X, Y);
        }
        T SquaredLength() const {
	        return X * X + Y * Y;
        }
        T Cross(const Vector2<T>& rhs) const {
	        return X * rhs.Y - Y * rhs.X;
        }
        T Dot(const Vector2<T>& vec) const {
	        return X * vec.X + Y * vec.Y;
        }
        T Distance(const Vector2<T>& rhs) const {
	        return (*this - rhs).Length();
        }
        Vector2<T>& NormalizeSelf() {
            T length = Hypot<T>(X, Y);
            if (!Math::FloatEqual(length, 0.0f)){
                T inv_length = (T)1 / length;
                X *= inv_length;
                Y *= inv_length;
            }
            return *this;
        }
        Vector2<T> Normalize() {
	        Vector2<T> r = { X, Y };
        	r.NormalizeSelf();
        	return r;
        }
        static Vector2<T> Max(const Vector2<T>& v0, const Vector2<T>& v1) {
	        return { Math::Max<T>(v0.X, v1.X), Math::Max<T>(v0.Y, v1.Y) };
        }
        static Vector2<T> Min(const Vector2<T>& v0, const Vector2<T>& v1) {
            return { Math::Min<T>(v0.X, v1.X), Math::Min<T>(v0.Y, v1.Y) };
        }
        static bool AllLess(const Vector2<T>& v0, const Vector2<T>& v1) {
            return v0.X < v1.X && v0.Y < v1.Y;
        }
        static bool AllLessOrEqual(const Vector2<T>& v0, const Vector2<T>& v1) {
            return v0.X <= v1.X && v0.Y <= v1.Y;
        }
        static bool AllGreater(const Vector2<T>& v0, const Vector2<T>& v1) {
            return v0.X > v1.X && v0.Y > v1.Y;
        }
        static bool AllGreaterOrEqual(const Vector2<T>& v0, const Vector2<T>& v1) {
            return v0.X >= v1.X && v0.Y >= v1.Y;
        }
    };

    template <typename T> const Vector2<T> Vector2<T>::ZERO{ 0, 0 };
     
    template <typename T> struct Vector3 {
        T X{ 0.0f };
        T Y{ 0.0f };
    	T Z{ 0.0f };
        static const Vector3<T> ZERO;
        static const Vector3<T> ONE;
        Vector3() = default;
        Vector3(T scalar) :X(scalar), Y(scalar), Z(scalar) {}
        Vector3(T _x, T _y, T _z) :X(_x), Y(_y), Z(_z) {}
        explicit Vector3(const T* coords) : X{ coords[0] }, Y{ coords[1] }, Z{ coords[2] } {}

        T* Data() {
	        return &X;
        }
        const T* Data() const {
	        return &X;
        }

        T operator[](int i) const {
	        return *(&X + i);
        }
        T& operator[](int i) {
	        return *(&X + i);
        }
        bool operator==(const Vector3<T>& rhs) const {
	        return (X == rhs.X && Y == rhs.Y && Z == rhs.Z);
        }
        bool operator!=(const Vector3<T>& rhs) const {
	        return X != rhs.X || Y != rhs.Y || Z != rhs.Z;
        }
        // arithmetic operations
        Vector3<T> operator+(const Vector3<T>& rhs) const {
	        return Vector3<T>(X + rhs.X, Y + rhs.Y, Z + rhs.Z);
        }
        Vector3<T> operator-(const Vector3<T>& rhs) const {
	        return Vector3<T>(X - rhs.X, Y - rhs.Y, Z - rhs.Z);
        }
        Vector3<T> operator-() const { return Vector3<T>{-X, -Y, -Z}; }
        Vector3<T> operator*(T scalar) const {
	        return Vector3<T>(X * scalar, Y * scalar, Z * scalar);
        }
        Vector3<T> operator*(const Vector3<T>& rhs) const {
	        return Vector3<T>(X * rhs.X, Y * rhs.Y, Z * rhs.Z);
        }
        Vector3<T> operator/(T scalar) const {
	        return Vector3<T>(X / scalar, Y / scalar, Z / scalar);
        }
        Vector3<T> operator/(const Vector3<T>& rhs) const {
	        return Vector3<T>(X / rhs.X, Y / rhs.Y, Z / rhs.Z);
        }
        // overloaded operators to help Vector3<T>
        friend Vector3<T> operator*(T scalar, const Vector3<T>& rhs) {
	        return Vector3<T>(scalar * rhs.X, scalar * rhs.Y, scalar * rhs.Z);
        }
        friend Vector3<T> operator+(const Vector3<T>& lhs, T rhs) {
	        return Vector3<T>(lhs.X + rhs, lhs.Y + rhs, lhs.Z + rhs);
        }
        friend Vector3<T> operator+(T lhs, const Vector3<T>& rhs) {
	        return Vector3<T>(lhs + rhs.X, lhs + rhs.Y, lhs + rhs.Z);
        }
        friend Vector3<T> operator-(const Vector3<T>& lhs, T rhs) {
	        return Vector3<T>(lhs.X - rhs, lhs.Y - rhs, lhs.Z - rhs);
        }
        friend Vector3<T> operator-(T lhs, const Vector3<T>& rhs) {
	        return Vector3<T>(lhs - rhs.X, lhs - rhs.Y, lhs - rhs.Z);
        }

        // arithmetic updates
        Vector3<T>& operator+=(const Vector3<T>& rhs) {
            X += rhs.X;
            Y += rhs.Y;
            Z += rhs.Z;
            return *this;
        }
        Vector3<T>& operator+=(T scalar) {
            X += scalar;
            Y += scalar;
            Z += scalar;
            return *this;
        }
        Vector3<T>& operator-=(const Vector3<T>& rhs) {
            X -= rhs.X;
            Y -= rhs.Y;
            Z -= rhs.Z;
            return *this;
        }
        Vector3<T>& operator-=(T scalar) {
            X -= scalar;
            Y -= scalar;
            Z -= scalar;
            return *this;
        }
        Vector3<T>& operator*=(T scalar) {
            X *= scalar;
            Y *= scalar;
            Z *= scalar;
            return *this;
        }
        Vector3<T>& operator*=(const Vector3<T>& rhs) {
            X *= rhs.X;
            Y *= rhs.Y;
            Z *= rhs.Z;
            return *this;
        }
        Vector3<T>& operator/=(T scalar) {
            X /= scalar;
            Y /= scalar;
            Z /= scalar;
            return *this;
        }
        Vector3<T>& operator/=(const Vector3<T>& rhs) {
            X /= rhs.X;
            Y /= rhs.Y;
            Z /= rhs.Z;
            return *this;
        }
        T Length() const {
            return Hypot<T>(X, Y, Z);
        }
        T LengthSquared() const {
	        return X * X + Y * Y + Z * Z;
        }
        T Distance(const Vector3<T>& rhs) const {
	        return (*this - rhs).Length();
        }
        T DistanceSquared(const Vector3<T>& rhs) const {
	        return (*this - rhs).LengthSquared();
        }
        T Dot(const Vector3<T>& rhs) const {
	        return X * rhs.X + Y * rhs.Y + Z * rhs.Z;
        }
        Vector3<T> Cross(const Vector3<T>& rhs) const {
	        return Vector3<T>(Y * rhs.Z - Z * rhs.Y, Z * rhs.X - X * rhs.Z, X * rhs.Y - Y * rhs.X);
        }
    	void NormalizeSelf() {
            T length = Hypot<T>(X, Y, Z);
            if (!Math::FloatEqual(length, 0.0)) {
                T lengthInv = (T)1 / length;
                X *= lengthInv;
                Y *= lengthInv;
                Z *= lengthInv;
            }
        }
        Vector3<T> Normalize() const {
	        Vector3<T> r{ X, Y, Z };
        	r.NormalizeSelf();
        	return r;
        }

        static T Dot(const Vector3<T>& v0, const Vector3<T>& v1) {
	        return v0.Dot(v1);
        }
        static Vector3<T> Cross(const Vector3<T>& v0, const Vector3<T>& v1) {
	        return v0.Cross(v1);
        }
        static Vector3<T> Max(const Vector3<T>& v0, const Vector3<T>& v1) {
            return { Math::Max<T>(v0.X, v1.X), Math::Max<T>(v0.Y, v1.Y), Math::Max<T>(v0.Z, v1.Z) };
        }
        static Vector3<T> Min(const Vector3<T>& v0, const Vector3<T>& v1) {
            return { Math::Min<T>(v0.X, v1.X), Math::Min<T>(v0.Y, v1.Y), Math::Min<T>(v0.Z, v1.Z) };
        }
        static Vector3<T> Abs(const Vector3<T>& v) {
            return { Math::Abs(v.X), Math::Abs(v.Y), Math::Abs(v.Z) };
        }
        static bool AllLess(const Vector3<T>& v0, const Vector3<T>& v1) {
            return v0.X < v1.X && v0.Y < v1.Y && v0.Z < v1.Z;
        }
        static bool AllLessOrEqual(const Vector3<T>& v0, const Vector3<T>& v1) {
            return v0.X <= v1.X && v0.Y <= v1.Y && v0.Z <= v1.Z;
        }
        static bool AllGreater(const Vector3<T>& v0, const Vector3<T>& v1) {
            return v0.X > v1.X && v0.Y > v1.Y && v0.Z > v1.Z;
        }
        static bool AllGreaterOrEqual(const Vector3<T>& v0, const Vector3<T>& v1) {
            return v0.X >= v1.X && v0.Y >= v1.Y && v0.Z >= v1.Z;
        }
    };

    template<typename T> const Vector3<T> Vector3<T>::ZERO{ 0,0,0 };
    template<typename T> const Vector3<T> Vector3<T>::ONE{ 1,1,1 };

    template <typename T> struct Vector4 {
        T X{ 0.f };
        T Y{ 0.f };
        T Z{ 0.f };
    	T W{ 0.f };
        static const Vector4<T> ZERO;

        Vector4() = default;
        Vector4(T _x, T _y, T _z, T _w) : X{ _x }, Y{ _y }, Z{ _z }, W{ _w } {}
        Vector4(const Vector3<T>& v3, T w_) : X{ v3.X }, Y{ v3.Y }, Z{ v3.Z }, W{ w_ } {}
        explicit Vector4(T* coords) : X{ coords[0] }, Y{ coords[1] }, Z{ coords[2] }, W{ coords[3] } {}

        T* Data() {
	        return &X;
        }
        const T* Data() const {
	        return &X;
        }

        T operator[](int i) const {
	        return *(&X + i);
        }
        T& operator[](int i) {
	        return *(&X + i);
        }
        bool operator==(const Vector4<T> & rhs) const {
	        return (X == rhs.X && Y == rhs.Y && Z == rhs.Z && W == rhs.W);
        }
        bool operator!=(const Vector4<T> & rhs) const {
	        return !(rhs == *this);
        }
        Vector4<T> operator+(const Vector4<T> & rhs) const {
	        return Vector4<T>(X + rhs.X, Y + rhs.Y, Z + rhs.Z, W + rhs.W);
        }
        Vector4<T> operator-(const Vector4<T> & rhs) const {
	        return Vector4<T>(X - rhs.X, Y - rhs.Y, Z - rhs.Z, W - rhs.W);
        }
        Vector4<T> operator*(T scalar) const {
	        return Vector4<T>(X * scalar, Y * scalar, Z * scalar, W * scalar);
        }
        Vector4<T> operator*(const Vector4<T> & rhs) const {
	        return Vector4<T>(rhs.X * X, rhs.Y * Y, rhs.Z * Z, rhs.W * W);
        }
        Vector4<T> operator/(T scalar) const {
	        return Vector4<T>(X / scalar, Y / scalar, Z / scalar, W / scalar);
        }
        Vector4<T> operator/(const Vector4<T>& rhs) const { return Vector4<T>(X / rhs.X, Y / rhs.Y, Z / rhs.Z, W / rhs.W); }
        friend Vector4<T> operator*(T scalar, const Vector4<T> & rhs) {
	        return Vector4<T>(scalar * rhs.X, scalar * rhs.Y, scalar * rhs.Z, scalar * rhs.W);
        }
        friend Vector4<T> operator+(const Vector4<T> & lhs, T rhs) {
	        return Vector4<T>(lhs.X + rhs, lhs.Y + rhs, lhs.Z + rhs, lhs.W + rhs);
        }
        friend Vector4<T> operator+(T lhs, const Vector4<T> & rhs) {
	        return Vector4<T>(lhs + rhs.X, lhs + rhs.Y, lhs + rhs.Z, lhs + rhs.W);
        }
        friend Vector4<T> operator-(const Vector4<T> & lhs, T rhs) {
	        return Vector4<T>(lhs.X - rhs, lhs.Y - rhs, lhs.Z - rhs, lhs.W - rhs);
        }
        friend Vector4<T> operator-(T lhs, const Vector4<T> & rhs) {
	        return Vector4<T>(lhs - rhs.X, lhs - rhs.Y, lhs - rhs.Z, lhs - rhs.W);
        }
        // arithmetic updates
        Vector4<T>& operator+=(const Vector4<T>& rhs) {
            X += rhs.X;
            Y += rhs.Y;
            Z += rhs.Z;
            W += rhs.W;
            return *this;
        }
        Vector4<T>& operator-=(const Vector4<T>& rhs) {
            X -= rhs.X;
            Y -= rhs.Y;
            Z -= rhs.Z;
            W -= rhs.W;
            return *this;
        }
        Vector4<T>& operator*=(T scalar) {
            X *= scalar;
            Y *= scalar;
            Z *= scalar;
            W *= scalar;
            return *this;
        }
        Vector4<T>& operator+=(T scalar) {
            X += scalar;
            Y += scalar;
            Z += scalar;
            W += scalar;
            return *this;
        }
        Vector4<T>& operator-=(T scalar) {
            X -= scalar;
            Y -= scalar;
            Z -= scalar;
            W -= scalar;
            return *this;
        }
        Vector4<T>& operator*=(const Vector4<T>& rhs) {
            X *= rhs.X;
            Y *= rhs.Y;
            Z *= rhs.Z;
            W *= rhs.W;
            return *this;
        }
        Vector4<T>& operator/=(T scalar) {
            X /= scalar;
            Y /= scalar;
            Z /= scalar;
            W /= scalar;
            return *this;
        }
        Vector4<T>& operator/=(const Vector4<T>& rhs) {
            X /= rhs.X;
            Y /= rhs.Y;
            Z /= rhs.Z;
            W /= rhs.W;
            return *this;
        }
        T Dot(const Vector4<T> & vec) const {
	        return X * vec.X + Y * vec.Y + Z * vec.Z + W * vec.W;
        }

        static Vector4<T> Max(const Vector4<T>& v0, const Vector4<T>& v1) {
            return { Math::Max<T>(v0.X, v1.X), Math::Max<T>(v0.Y, v1.Y), Math::Max<T>(v0.Z, v1.Z), Math::Max<T>(v0.W, v1.W) };
        }
        static Vector4<T> Min(const Vector4<T>& v0, const Vector4<T>& v1) {
            return { Math::Min<T>(v0.X, v1.X), Math::Min<T>(v0.Y, v1.Y), Math::Min<T>(v0.Z, v1.Z), Math::Min<T>(v0.W, v1.W) };
        }
    };

    template<typename T> const Vector4<T> Vector4<T>::ZERO{ 0,0,0,0 };

    typedef Vector2<float>         FVector2;
    typedef Vector2<double>        DVector2;
    typedef Vector2<int>           IVector2;

    typedef Vector3<float>         FVector3;
    typedef Vector3<double>        DVector3;
    typedef Vector3<int>	       IVector3;

    typedef Vector4<float>         FVector4;
    typedef Vector4<double>        DVector4;
    typedef Vector4<int>	       IVector4;
}