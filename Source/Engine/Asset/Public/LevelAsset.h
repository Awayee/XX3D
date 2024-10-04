#pragma once
#include "AssetCommon.h"
#include "Math/Public/Vector.h"
#include "Core/Public/Json.h"

namespace Asset {
	struct LevelAsset : AssetBase {
		struct MeshData {
			XString Name;
			XString File;
			Math::FVector3 Position;
			Math::FVector3 Scale;
			Math::FVector3 Rotation;
		};
		TArray<MeshData> Meshes;

		struct InstancedMeshData {
			XString Name;
			XString MeshFile;
			XString InstanceFile;
		};
		TArray<InstancedMeshData> InstancedMeshes;

		struct{
			Math::FVector3 Eye;
			Math::FVector3 At;
			Math::FVector3 Up;
			int ProjType;
			float Near, Far, Fov, ViewSize;
		}CameraData;

		struct{
			Math::FVector3 Rotation;
			Math::FVector3 Color;
		}DirectionalLightData;

		XString SkyBox;
		bool Load(File::PathStr filePath) override;
		bool Save(File::PathStr filePath) override;
	};
}
