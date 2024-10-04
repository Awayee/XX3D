#pragma once
#include "AssetCommon.h"
#include "Math/Public/Geometry.h"
#include "Math/Public/Transform.h"

namespace Asset {

	enum class EVertexFormat : int8 {
		UINT16,
		UINT32,
	};

	enum class EMeshCompressMode : int8 {
		NONE = 0,
		LZ4,
	};


	struct MeshAsset : public AssetBase {
		XString Name;
		struct SPrimitive {
			XString BinaryFile;
			XString MaterialFile;
			XString Name;
			Math::AABB3 AABB;
			TArray<XString> Textures;
			TArray<AssetVertex> Vertices;
			TArray<IndexType> Indices;
		};
		TArray<SPrimitive> Primitives;

	public:
		MeshAsset() = default;
		bool Load(File::PathStr filePath) override;
		bool Save(File::PathStr filePath) override;
		//load a packed primitive file
		static bool LoadPrimitiveFile(const char* file, TArray<AssetVertex>& vertices, TArray<IndexType>& indices);
		//export with pack, only execute when import external files
		static bool ExportPrimitiveFile(const char* file, const TArray<AssetVertex>& vertices, const TArray<IndexType>& indices, EMeshCompressMode packMode = EMeshCompressMode::NONE);
	};

	struct InstancedMeshAsset: public AssetBase {
		XString MeshFile;
		XString InstanceFile;
		static bool LoadInstanceFile(const char* file, TArray<Math::FTransform>& instances);
		static bool SaveInstanceFile(const char* file, TConstArrayView<Math::FTransform> instances);
		bool Load(File::PathStr filePath) override;
		bool Save(File::PathStr filePath) override;
	};
}
