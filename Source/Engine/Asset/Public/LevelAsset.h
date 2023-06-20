#pragma once
#include "AssetCommon.h"
#include "Core/Public/Math/Vector.h"
#include "Core/Public/Macro.h"

struct ALevelAsset : AAssetBase {
	struct MeshData {
		String Name;
		String File;
		Math::FVector3 Position;
		Math::FVector3 Scale;
		Math::FVector3 Euler;
	};
	TVector<MeshData> Meshes;

	struct {
		Math::FVector3 Eye;
		Math::FVector3 At;
		Math::FVector3 Up;
		float Near, Far, Fov;
	}CameraParam;
	bool Load(File::Read& in) override;
	bool Save(File::Write& out) override;
};
