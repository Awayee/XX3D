#pragma once
#include "AssetCommon.h"

namespace Engine {

	enum class EVertexFormat : int8 {
		UINT16,
		UINT32,
	};

	enum class EMeshCompressMode : int8 {
		NONE = 0,
		LZ4,
	};


	struct AMeshAsset : public AAssetBase {
	public:
		XXString Name;
		struct SPrimitive {
			XXString BinaryFile;
			XXString MaterialFile;
			XXString Name;
			TVector<XXString> Textures;
			TVector<FVertex> Vertices;
			TVector<IndexType> Indices;
		};
		TVector<SPrimitive> Primitives;

	public:
		AMeshAsset() = default;

		//load with primitive binaries
		bool Load(File::RFile& in) override;

		//save without primitives
		bool Save(File::WFile& out) override;

		//load a packed primitive file
		static bool LoadPrimitiveFile(const char* file, TVector<FVertex>& vertices, TVector<IndexType>& indices);

		//export with pack, only execute when import external files
		static bool ExportPrimitiveFile(const char* file, const TVector<FVertex>& vertices, const TVector<IndexType>& indices, EMeshCompressMode packMode = EMeshCompressMode::NONE);

	};
}
