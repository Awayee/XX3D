#pragma once
#include "Math/Public/Math.h"

namespace Object {
	class DirectionalLight {
	private:
		Math::FVector3 m_LightDir{-1,-1,-1};
		Math::FVector3 m_LightColor{0.7f,0.8f,0.8f};
	public:
		DirectionalLight() = default;
		void SetDir(const Math::FVector3& dir) { m_LightDir = dir; }
		void SetColor(const Math::FVector3& color) { m_LightColor = color; }
		const Math::FVector3& GetLightDir() const { return m_LightDir; }
		const Math::FVector3& GetLightColor() const { return m_LightColor; }
	};
}