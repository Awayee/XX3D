#pragma once
#include "Vector.h"
#include "Quaternion.h"
#include "Matrix.h"

namespace Math {
	template<typename T> struct Transform {
		Vector3<T> Position{ Vector3<T>::ZERO };
		Vector3<T> Scale{ Vector3<T>::ONE };
		Quaternion<T> Rotation{Quaternion<T>::IDENTITY};

		const static Transform<T> IDENTITY;
		static Transform MakeFromMatrix(const Math::Matrix4x4<T>& mat) {
			Transform transform;
			mat.Decomposition(transform.Position, transform.Scale, transform.Rotation);
			return transform;
		}

		Math::Matrix4x4<T> ToMatrix() {
			return Matrix4x4<T>::MakeTransform(Position, Scale, Rotation);
		}

		Transform Inverse() {
			Quaternion<T> invRotation = Rotation.Inverse();
			Vector3<T> invScale = {(T)1 / Scale.X, (T)1 / Scale.Y, (T)1 / Scale.Z};
			Vector3<T> scaledTranslation = invScale * Position;
			Vector3<T> t2 = invRotation.RotateVector3(scaledTranslation);
			Vector3<T> invTranslation = -t2;
			return Transform{ invTranslation, invScale, invRotation };
		}
	};
}
