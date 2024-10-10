#pragma once
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"

namespace Math {
	template<typename T> struct Transform {
		Vector3<T> Position{ 0,0,0 };
		Vector3<T> Scale{ 1,1,1 };
		Quaternion<T> Rotation{ 0,0,0,1 };

		static const Transform<T> IDENTITY;
		static Transform MakeFromMatrix(const Math::Matrix4x4<T>& mat) {
			Transform transform;
			mat.Decomposition(transform.Position, transform.Scale, transform.Rotation);
			return transform;
		}

		void BuildMatrix(Math::Matrix4x4<T>& mat) const {
			mat.MakeTransform(Position, Scale, Rotation);
		}

		void BuildInverseMatrix(Math::Matrix4x4<T>& mat) const {
			mat.MakeInverseTransform(Position, Scale, Rotation);
		}

		Math::Matrix4x4<T> ToMatrix() const {
			Math::Matrix4x4<T> matrix;
			matrix.MakeTransform(Position, Scale, Rotation);
			return matrix;
		}

		Transform Inverse() {
			Quaternion<T> invRotation = Rotation.Inverse();
			Vector3<T> invScale = {(T)1 / Scale.X, (T)1 / Scale.Y, (T)1 / Scale.Z};
			Vector3<T> scaledTranslation = invScale * Position;
			Vector3<T> t2 = invRotation.RotateVector3(scaledTranslation);
			Vector3<T> invTranslation = -t2;
			return Transform{ invTranslation, invScale, invRotation };
		}

		void Accumulate(const Transform& rhs) {
			Rotation = rhs.Rotation * Rotation;
			Scale *= rhs.Scale;
			Position += rhs.Position;
		}
	};

	template<typename T> const Transform<T> Transform<T>::IDENTITY{};

	typedef Transform<float> FTransform;
}
