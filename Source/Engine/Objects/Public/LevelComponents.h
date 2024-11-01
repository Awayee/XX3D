#pragma once
#include "Level.h"
#include "Math/Public/Vector.h"
namespace Object {
	class CameraComponent: public LevelComponent {
	public:
		union DirtyFlags {
			uint32 Value;
			struct {
				bool View : 1;
				bool Proj : 1;
			};
			DirtyFlags(uint32 value=0) : Value(value) {}
		};
		Math::FVector3 Eye;
		Math::FVector3 At;
		Math::FVector3 Up;
		EProjType ProjType;
		float Near;
		float Far;
		float Fov;
		float HalfHeight;
		void OnLoad(const Json::Value& val) override;
		void OnAdd() override;
		void OnRemove() override {}
		void SyncData(DirtyFlags flags = 0xffffffff);
		REGISTER_LEVEL_COMPONENT(CameraComponent);
	};

	class DirectionalLightComponent: public LevelComponent {
	public:
		union DirtyFlags {
			uint32 Value;
			struct {
				bool Light : 1;
				bool Shadow : 1;
			};
			DirtyFlags(uint32 value = 0) : Value(value) {}
		};
		// light
		Math::FVector3 Rotation;
		Math::FVector4 Color;
		// shadow
		bool EnableShadow;
		bool ShadowDebug;
		float ShadowDistance;
		float ShadowLogDistribution;
		uint32 ShadowMapSize;
		float ShadowBiasConst;
		float ShadowBiasSlope;
		void OnLoad(const Json::Value& val) override;
		void OnAdd() override;
		void OnRemove() override {}
		void SyncData(DirtyFlags flags=0xffffffff);
		REGISTER_LEVEL_COMPONENT(DirectionalLightComponent);
	};
}
