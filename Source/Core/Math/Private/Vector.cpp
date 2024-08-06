#include "Math/Public/MathBase.h"
#include "Math/Public/Vector.h"
#include "Core/Public/Log.h"

namespace Math {
    // vector2
    template<typename T> T Vector2<T>::operator[](int i) const{
        ASSERT(i < 2, "Index out of range!");
        return (i == 0 ? X : Y);
    }

    template<typename T> T& Vector2<T>::operator[](int i){
        CHECK(i < 2);
        return (i == 0 ? X : Y);
    }

    template <typename T>
    Vector2<T>& Vector2<T>::operator+=(const Vector2<T>& rhs) {
        X += rhs.X;
        Y += rhs.Y;
        return *this;
    }

    template <typename T>
    Vector2<T>& Vector2<T>::operator+=(T scalar) {
        X += scalar;
        Y += scalar;
        return *this;
    }

    template <typename T>
    Vector2<T>& Vector2<T>::operator-=(const Vector2<T>& rhs) {
        X -= rhs.X;
        Y -= rhs.Y;
        return *this;
    }

    template <typename T>
    Vector2<T>& Vector2<T>::operator-=(T scalar) {
        X -= scalar;
        Y -= scalar;
        return *this;
    }

    template <typename T>
    Vector2<T>& Vector2<T>::operator*=(T scalar) {
        X *= scalar;
        Y *= scalar;
        return *this;
    }

    template <typename T>
    Vector2<T>& Vector2<T>::operator*=(const Vector2<T>& rhs) {
        X *= rhs.X;
        Y *= rhs.Y;
        return *this;
    }

    template <typename T>
    Vector2<T>& Vector2<T>::operator/=(T scalar) {
        T inv = (T)1 / scalar;
        X *= inv;
        Y *= inv;
        return *this;
    }

    template <typename T>
    Vector2<T>& Vector2<T>::operator/=(const Vector2<T>& rhs) {
        X /= rhs.X;
        Y /= rhs.Y;
        return *this;
    }

    template<typename T> T Vector2<T>::Length() const{
        return Hypot<T>(X, Y);
    }

    template<typename T> T Vector2<T>::Normalize(){
        T lengh = Hypot<T>(X, Y);

        if (lengh > 0)
        {
            T inv_length = (T)1 / lengh;
            X *= inv_length;
            Y *= inv_length;
        }

        return lengh;
    }

    template<typename T> const Vector2<T> Vector2<T>::ZERO{ 0,0 };

    template<typename T> Vector2<T> Vector2<T>::Max(const Vector2<T>& v0, const Vector2<T>& v1){
        return { Math::Max<T>(v0.X, v1.X), Math::Max<T>(v0.Y, v1.Y) };
    }
    template<typename T> Vector2<T> Vector2<T>::Min(const Vector2<T>& v0, const Vector2<T>& v1){
        return { Math::Min<T>(v0.X, v1.X), Math::Min<T>(v0.Y, v1.Y) };
    }

    template<typename T> bool Vector2<T>::AllLess(const Vector2<T>& v0, const Vector2<T>& v1) {
        return v0.X < v1.X && v0.Y < v1.Y;
    }

    template<typename T> bool Vector2<T>::AllLessOrEqual(const Vector2<T>& v0, const Vector2<T>& v1) {
        return v0.X <= v1.X && v0.Y <= v1.Y;
    }

    template<typename T>  bool Vector2<T>::AllGreater(const Vector2<T>& v0, const Vector2<T>& v1) {
        return v0.X > v1.X && v0.Y > v1.Y;
    }

    template<typename T>  bool Vector2<T>::AllGreaterOrEqual(const Vector2<T>& v0, const Vector2<T>& v1) {
        return v0.X >= v1.X && v0.Y >= v1.Y;
    }

    // vector3
    template<typename T> T Vector3<T>::operator[](int i) const{
        ASSERT(i < 3, "Index out of range!");
        return *(&X + i);
    }
    template<typename T> T& Vector3<T>::operator[](int i){
        ASSERT(i < 3, "Index out of range!");
        return *(&X + i);
    }
    template<typename T> Vector3<T> Vector3<T>::operator/(T scalar) const{
        ASSERT(scalar != 0.0, "Divide by zero!");
        return Vector3<T>(X / scalar, Y / scalar, Z / scalar);
    }
    template<typename T> Vector3<T> Vector3<T>::operator/(const Vector3<T>& rhs) const{
        ASSERT((rhs.X != 0 && rhs.Y != 0 && rhs.Z != 0), "Divide by zero!");
        return Vector3<T>(X / rhs.X, Y / rhs.Y, Z / rhs.Z);
    }

    template <typename T>
    Vector3<T>& Vector3<T>::operator+=(const Vector3<T>& rhs) {
        X += rhs.X;
        Y += rhs.Y;
        Z += rhs.Z;
        return *this;
    }

    template <typename T>
    Vector3<T>& Vector3<T>::operator+=(T scalar) {
        X += scalar;
        Y += scalar;
        Z += scalar;
        return *this;
    }

    template <typename T>
    Vector3<T>& Vector3<T>::operator-=(const Vector3<T>& rhs) {
        X -= rhs.X;
        Y -= rhs.Y;
        Z -= rhs.Z;
        return *this;
    }

    template <typename T>
    Vector3<T>& Vector3<T>::operator-=(T scalar) {
        X -= scalar;
        Y -= scalar;
        Z -= scalar;
        return *this;
    }

    template <typename T>
    Vector3<T>& Vector3<T>::operator*=(T scalar) {
        X *= scalar;
        Y *= scalar;
        Z *= scalar;
        return *this;
    }

    template <typename T>
    Vector3<T>& Vector3<T>::operator*=(const Vector3<T>& rhs) {
        X *= rhs.X;
        Y *= rhs.Y;
        Z *= rhs.Z;
        return *this;
    }

    template<typename T> Vector3<T>& Vector3<T>::operator/=(T scalar){
        CHECK(scalar != 0.0);
        X /= scalar;
        Y /= scalar;
        Z /= scalar;
        return *this;
    }
    template<typename T> Vector3<T>& Vector3<T>::operator/=(const Vector3<T>& rhs){
        CHECK(rhs.X != 0 && rhs.Y != 0 && rhs.Z != 0);
        X /= rhs.X;
        Y /= rhs.Y;
        Z /= rhs.Z;
        return *this;
    }
    template<typename T> Vector3<T> operator/(T scalar, const Vector3<T>& rhs){
        CHECK(rhs.X != 0 && rhs.Y != 0 && rhs.Z != 0);
        return Vector3(scalar / rhs.X, scalar / rhs.Y, scalar / rhs.Z);
    }
    template<typename T> T Vector3<T>::Length() const{
        return Hypot<T>(X, Y, Z);
    }
    template<typename T> void Vector3<T>::Normalize(){
        T length = Hypot<T>(X, Y, Z);
        if (length == 0)
            return;

        T lengthInv = (T)1/ length;
        X *= lengthInv;
        Y *= lengthInv;
        Z *= lengthInv;
    }

    template<typename T> const Vector3<T> Vector3<T>::ZERO{ 0,0,0 };
    template<typename T> Vector3<T> Vector3<T>::Max(const Vector3<T>& v0, const Vector3<T>& v1){
        return { Math::Max<T>(v0.X, v1.X), Math::Max<T>(v0.Y, v1.Y), Math::Max<T>(v0.Z, v1.Z) };
    }
    template<typename T> Vector3<T> Vector3<T>::Min(const Vector3<T>& v0, const Vector3<T>& v1){
        return { Math::Min<T>(v0.X, v1.X), Math::Min<T>(v0.Y, v1.Y), Math::Min<T>(v0.Z, v1.Z) };
    }

    template<typename T> Vector3<T> Vector3<T>::Abs(const Vector3<T>& v) {
        return { Math::Abs(v.X), Math::Abs(v.Y), Math::Abs(v.Z) };
    }

    template<typename T> bool Vector3<T>::AllLess(const Vector3<T>& v0, const Vector3<T>& v1) {
        return v0.X < v1.X && v0.Y < v1.Y && v0.Z < v1.Z;
    }

    template<typename T> bool Vector3<T>::AllLessOrEqual(const Vector3<T>& v0, const Vector3<T>& v1) {
        return v0.X <= v1.X&& v0.Y <= v1.Y&& v0.Z <= v1.Z;
    }

    template<typename T> bool Vector3<T>::AllGreater(const Vector3<T>& v0, const Vector3<T>& v1) {
        return v0.X > v1.X && v0.Y > v1.Y && v0.Z > v1.Z;
    }

    template<typename T> bool Vector3<T>::AllGreaterOrEqual(const Vector3<T>& v0, const Vector3<T>& v1) {
        return v0.X >= v1.X && v0.Y >= v1.Y && v0.Z >= v1.Z;
    }

    // vector4
    template<typename T> T Vector4<T>::operator[](int i) const{
        CHECK(i < 4);
        return *(&X + i);
    }
    template<typename T> T& Vector4<T>::operator[](int i){
        CHECK(i < 4);
        return *(&X + i);
    }
    template<typename T> Vector4<T> Vector4<T>::operator/(T scalar) const{
        CHECK(scalar != 0.0);
        return Vector4<T>(X / scalar, Y / scalar, Z / scalar, W / scalar);
    }
    template<typename T> Vector4<T> Vector4<T>::operator/(const Vector4<T>& rhs) const{
        CHECK(rhs.X != 0 && rhs.Y != 0 && rhs.Z != 0 && rhs.W != 0);
        return Vector4<T>(X / rhs.X, Y / rhs.Y, Z / rhs.Z, W / rhs.W);
    }
    template<typename T> Vector4<T> operator/(T scalar, const Vector4<T>& rhs){
        CHECK(rhs.X != 0 && rhs.Y != 0 && rhs.Z != 0 && rhs.W != 0);
        return Vector4(scalar / rhs.X, scalar / rhs.Y, scalar / rhs.Z, scalar / rhs.W);
    }

    template <typename T>
    Vector4<T>& Vector4<T>::operator+=(const Vector4<T>& rhs) {
        X += rhs.X;
        Y += rhs.Y;
        Z += rhs.Z;
        W += rhs.W;
        return *this;
    }

    template <typename T>
    Vector4<T>& Vector4<T>::operator-=(const Vector4<T>& rhs) {
        X -= rhs.X;
        Y -= rhs.Y;
        Z -= rhs.Z;
        W -= rhs.W;
        return *this;
    }

    template <typename T>
    Vector4<T>& Vector4<T>::operator*=(T scalar) {
        X *= scalar;
        Y *= scalar;
        Z *= scalar;
        W *= scalar;
        return *this;
    }

    template <typename T>
    Vector4<T>& Vector4<T>::operator+=(T scalar) {
        X += scalar;
        Y += scalar;
        Z += scalar;
        W += scalar;
        return *this;
    }

    template <typename T>
    Vector4<T>& Vector4<T>::operator-=(T scalar) {
        X -= scalar;
        Y -= scalar;
        Z -= scalar;
        W -= scalar;
        return *this;
    }

    template <typename T>
    Vector4<T>& Vector4<T>::operator*=(const Vector4<T>& rhs) {
        X *= rhs.X;
        Y *= rhs.Y;
        Z *= rhs.Z;
        W *= rhs.W;
        return *this;
    }

    template<typename T> Vector4<T>& Vector4<T>::operator/=(T scalar){
        CHECK(scalar != 0.0);
        X /= scalar;
        Y /= scalar;
        Z /= scalar;
        W /= scalar;
        return *this;
    }
    template<typename T> Vector4<T>& Vector4<T>::operator/=(const Vector4<T>& rhs){
        CHECK(rhs.X != 0 && rhs.Y != 0 && rhs.Z != 0);
        X /= rhs.X;
        Y /= rhs.Y;
        Z /= rhs.Z;
        W /= rhs.W;
        return *this;
    }

    template<typename T> const Vector4<T> Vector4<T>::ZERO{ 0,0,0,0 };

    template<typename T> Vector4<T> Vector4<T>::Max(const Vector4<T>& v0, const Vector4<T>& v1){
        return { Math::Max<T>(v0.X, v1.X), Math::Max<T>(v0.Y, v1.Y), Math::Max<T>(v0.Z, v1.Z), Math::Max<T>(v0.W, v1.W)};
    }
    template<typename T> Vector4<T> Vector4<T>::Min(const Vector4<T>& v0, const Vector4<T>& v1){
        return { Math::Min<T>(v0.X, v1.X), Math::Min<T>(v0.Y, v1.Y), Math::Min<T>(v0.Z, v1.Z), Math::Min<T>(v0.W, v1.W)};
    }

    template Vector2<float>;
    template Vector2<double>;
    template Vector2<int>;

    template Vector3<float>;
    template Vector3<double>;
    template Vector3<int>;

    template Vector4<float>;
    template Vector4<double>;
    template Vector4<int>;
}