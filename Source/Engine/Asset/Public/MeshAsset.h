#pragma once
#include "AssetCommon.h"

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
	public:
		XString Name;
		struct SPrimitive {
			XString BinaryFile;
			XString MaterialFile;
			XString Name;
			TArray<XString> Textures;
			TArray<AssetVertex> Vertices;
			TArray<IndexType> Indices;
		};
		TArray<SPrimitive> Primitives;

	public:
		MeshAsset() = default;

		//load with primitive binaries
		bool Load(File::RFile& in) override;

		//save without primitives
		bool Save(File::WFile& out) override;

		//load a packed primitive file
		static bool LoadPrimitiveFile(const char* file, TArray<AssetVertex>& vertices, TArray<IndexType>& indices);

		//export with pack, only execute when import external files
		static bool ExportPrimitiveFile(const char* file, const TArray<AssetVertex>& vertices, const TArray<IndexType>& indices, EMeshCompressMode packMode = EMeshCompressMode::NONE);

	};
}
