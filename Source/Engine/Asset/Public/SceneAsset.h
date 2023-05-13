#pragma once
#include "AssetCommon.h"
#include "Core/Public/Math/Vector.h"
#include "Core/Public/Macro.h"

struct ASceneAsset : AAssetBase {
	struct MeshData {
		String Name;
		String File;
		Math::FVector3 Position;
		Math::FVector3 Scale;
		Math::FVector3 Euler;
	};
	TVector<MeshData> Objects;

	struct {
		Math::FVector3 Eye;
		Math::FVector3 At;
		Math::FVector3 Up;
		float Near, Far, Fov;
	}CameraParam;
	bool Load(const char* file) override;
	bool Save(const char* file) override;
};
