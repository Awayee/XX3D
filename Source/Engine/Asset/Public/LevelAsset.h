#pragma once
#include "AssetCommon.h"
#include "Math/Public/Vector.h"
#include "Math/Public/Quaternion.h"
#include "Core/Public/Log.h"

namespace Asset {
	struct LevelAsset : AssetBase {
		struct MeshData {
			XString Name;
			XString File;
			Math::FVector3 Position;
			Math::FVector3 Scale;
			Math::FVector3 Rotation;
		};
		TVector<MeshData> Meshes;

		struct {
			Math::FVector3 Eye;
			Math::FVector3 At;
			Math::FVector3 Up;
			float Near, Far, Fov;
		}CameraParam;
		bool Load(File::RFile& in) override;
		bool Save(File::WFile& out) override;
	};
}
