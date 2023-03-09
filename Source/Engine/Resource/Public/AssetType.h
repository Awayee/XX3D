#pragma once
#include "AssetData.h"
#include "Core/Public/Math/Vector.h"
#include "Core/Public/Macro.h"
#include "Core/Public/TypeDefine.h"

namespace Engine {
	struct AAssetBase {
		virtual void Load(const char* file) = 0;
		virtual void Save(const char* file) = 0;
		virtual ~AAssetBase() = default;
	};

	struct AImageAsset : AAssetBase {
		int Width;
		int Height;
		int Channels;
		uint8* Pixels;
		void Load(const char* file) override;
		void Save(const char* file) override;
		~AImageAsset() override;
	};

	// models from external files, such as .fbx, .glb

	struct AExternalMeshAsset : AAssetBase {
		TVector<SPrimitiveData> Primitives;
		void Load(const char* file) override;
		void Save(const char* file) override {}
	};

	struct AMeshAsset : AAssetBase {

	};

	struct AFontAsset : AAssetBase {
		String Path;
		void Load(const char* file) override { Path = file; }
		void Save(const char* file) override {}
	};

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
	
}
