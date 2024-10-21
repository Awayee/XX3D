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

	struct PrimitiveAsset: public AssetBase {
		TArray<AssetVertex> Vertices;
		TArray<IndexType> Indices;
		inline static constexpr EMeshCompressMode CompressMode{ EMeshCompressMode::LZ4 };
		bool Load(File::PathStr filePath) override;
		bool Save(File::PathStr filePath) override;
	};

	struct MeshAsset : public AssetBase {
		XString Name;
		struct SPrimitive {
			XString BinaryFile;
			XString MaterialFile;
			XString Name;
			TArray<XString> Textures;
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

	struct InstanceDataAsset: public AssetBase {
		TArray<Math::FTransform> Instances;
		bool Load(File::PathStr filePath) override;
		bool Save(File::PathStr filePath) override;
	};
}
