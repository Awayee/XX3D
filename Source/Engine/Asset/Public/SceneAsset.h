#pragma once
#include "AssetCommon.h"
#include "Math/Public/Vector.h"
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
	void Load(const char* file) override;
	void Save(const char* file) override;
};
