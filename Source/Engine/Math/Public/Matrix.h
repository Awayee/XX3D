#pragma once
#include "Vector.h"
#include "Quaternion.h"

namespace Math {
    template<typename T> struct Matrix3x3 {
		T Mat[3][3];
		Matrix3x3() = default;
        explicit Matrix3x3(T** arr);
        Matrix3x3(T* arr);
        Matrix3x3(
            T entry00, T entry01, T entry02,
            T entry10, T entry11, T entry12,
            T entry20, T entry21, T entry22);

        Matrix3x3(const Vector3<T>& row0, const Vector3<T>& row1, const Vector3<T>& row2);
        // assignment and comparison
        T* operator[](int row_index) { return Mat[row_index]; }
        const T* operator[](int row_index) const { return Mat[row_index]; }
        bool operator==(const Matrix3x3<T>& rhs) const;
        bool operator!=(const Matrix3x3<T>& rhs) const { return !operator==(rhs); }
        // arithmetic operations
        Matrix3x3<T> operator+(const Matrix3x3<T>& rhs) const;
        Matrix3x3<T> operator-(const Matrix3x3<T>& rhs) const;
        Matrix3x3<T> operator-() const;
        Matrix3x3<T> operator*(const Matrix3x3<T>& rhs) const;
        // matrix * vector [3x3 * 3x1 = 3x1]
        Vector3<T> operator*(const Vector3<T>& rhs) const;
        // matrix * scalar
        Matrix3x3<T> operator*(T scalar) const;

        // scalar * matrix
        friend Matrix3x3<T> operator*(T scalar, const Matrix3x3<T>& rhs);

        // utilities
        Matrix3x3<T> Transpose() const;

        float Determinant() const;
        bool Inverse(Matrix3x3<T>& inv_mat, T fTolerance = 1e-06) const;

        Matrix3x3<T> Inverse(T tolerance = 1e-06);
        static const Matrix3x3<T> IDENTITY;
	};

    typedef Matrix3x3<float>  FMatrix3x3;
    typedef Matrix3x3<double> DMatrix3x3;
    typedef Matrix3x3<int>    IMatrix3x3;

    template<typename T> void MatrixToQuaternion(const Matrix3x3<T>& m, Quaternion<T>& q);

    template<typename T> void QuaternionToMatrix(const Quaternion<T>& q, Matrix3x3<T>& m);

    template<typename T> struct Matrix4x4
    {
        /// The matrix entries, indexed by [row][col]
        T Mat[4][4];

        /** Default constructor.
        @note
        It does <b>NOT</b> initialize the matrix for efficiency.
        */
        Matrix4x4() = default;
        Matrix4x4(const Matrix4x4<T>& mat);
        Matrix4x4(const T* matrixArray);
        Matrix4x4(T m00, T m01, T m02, T m03,
            T m10, T m11, T m12, T m13,
            T m20, T m21, T m22, T m23,
            T m30, T m31, T m32, T m33);

        Matrix4x4(const Vector4<T>& row0, const Vector4<T>& row1, const Vector4<T>& row2, const Vector4<T>& row3);

        static const Matrix4x4<T> IDENTITY;
        static const Matrix4x4<T> ZERO;

        T* operator[](int row_index);

        const T* operator[](int row_index) const;

        /** Matrix concatenation using '*'.
         */
        Matrix4x4<T> operator*(const Matrix4x4<T>& m2) const;

        /** Vector transformation using '*'.
        @remarks
        Transforms the given 3-D vector by the matrix, projecting the
        result back into <i>w</i> = 1.
        @note
        This means that the initial <i>w</i> is considered to be 1.0,
        and then all the three elements of the resulting 3-D vector are
        divided by the resulting <i>w</i>.
        */
        Vector4<T> operator*(const Vector4<T>& v) const;

        /** Matrix addition.
         */
        Matrix4x4<T> operator+(const Matrix4x4<T>& m2) const;

        /** Matrix subtraction.
         */
        Matrix4x4<T> operator-(const Matrix4x4<T>& m2) const;

        Matrix4x4<T> operator*(T scalar) const;

        /** Tests 2 matrices for equality.
         */
        bool operator==(const Matrix4x4<T>& m2) const;

        /** Tests 2 matrices for inequality.
         */
        bool operator!=(const Matrix4x4<T>& m2) const;
        /** Creates a standard 4x4 transformation matrix with a zero translation part from a rotation/scaling 3x3
 * matrix.
 */
        void SetMatrix3x3(const Matrix3x3<T>& mat3);
        Matrix4x4<T> Transpose() const;

        //-----------------------------------------------------------------------

        /*
        -----------------------------------------------------------------------
        Translation Transformation
        -----------------------------------------------------------------------
        */
        /** Sets the translation transformation part of the matrix.
         */
        void SetTranslate(const Vector3<T>& v);

        /** Extracts the translation transformation part of the matrix.
         */
        Vector3<T> GetTranslate() const { return Vector3<T>(Mat[0][3], Mat[1][3], Mat[2][3]); }

        Matrix4x4<T> BuildViewportMatrix(unsigned int width, unsigned int height);

        static Matrix4x4<T> MirrorMatrix(Vector4<T> mirror);

        static Matrix4x4<T> RotationMatrix(Vector3<T> normal);

        /** Builds a translation matrix
         */
        void MakeTrans(const Vector3<T>& v);

        void MakeTrans(T tx, T ty, T tz);

        /*
        -----------------------------------------------------------------------
        Scale Transformation
        -----------------------------------------------------------------------
        */
        /** Sets the scale part of the matrix.
         */
        void SetScale(const Vector3<T>& v);

        /** Gets a scale matrix - variation for not using a vector.
         */
        static Matrix4x4<T> BuildScaleMatrix(T s_x, T s_y, T s_z);

        /** Extracts the rotation / scaling part of the Matrix as a 3x3 matrix.
        @param m3x3 Destination Matrix3
        */
        void ExtractMatrix3x3(Matrix3x3<T>& m3x3);

        void ExtractAxies(Vector3<T>& outX, Vector3<T>& outY, Vector3<T>& outZ);

        /** Building a Matrix4 from orientation / scale / position.
        @remarks
        Transform is performed in the order scale, rotate, translation, i.e. translation is independent
        of orientation axes, scale does not affect size of translation, rotation and scaling are always
        centered on the origin.
        */
        void MakeTransform(const Vector3<T>& position, const Vector3<T>& scale, const Quaternion<T>& rotation);


        /** Building an inverse Matrix4 from orientation / scale / position.
        @remarks
        As MakeTransform except it build the inverse given the same data as MakeTransform, so
        performing -translation, -rotate, 1/scale in that order.
        */
        void MakeInverseTransform(const Vector3<T>& position, const Vector3<T>& scale, const Quaternion<T>& rotation);

        /** Decompose a Matrix4 to orientation / scale / position.
         */
        void Decomposition(Vector3<T>& position, Vector3<T>& scale, Quaternion<T>& rotate) const;

        Matrix4x4<T> Inverse() const;

        Vector3<T> TransformCoord(const Vector3<T>& v);

        static Matrix4x4<T> ViewMatrix(const Vector3<T>& position, const Quaternion<T>& orientation, const Matrix4x4<T>* reflectMat = nullptr);

        static Matrix4x4<T> LookAtMatrix(const Vector3<T>& eye, const Vector3<T>& at, const Vector3<T>& up);

        static Matrix4x4<T> PerspectiveMatrix(T fov, T aspect, T zNear, T zFar);

        static Matrix4x4<T> OrthographicMatrix(T left, T right, T bottom, T top, T znear, T zfar);
    };

    typedef Matrix4x4<float>  FMatrix4x4;
    typedef Matrix4x4<double> DMatrix4x4;
    typedef Matrix4x4<int>    IMatrix4x4;
}