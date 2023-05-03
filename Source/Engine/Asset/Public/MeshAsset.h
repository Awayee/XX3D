#pragma once
#include "AssetCommon.h"
#include "AssetMgr.h"

enum class EVertexFormat: int8 {
	UINT16,
	UINT32,
};

enum class EPackMode: int8 {
	NONE=0,
	LZ4,
};


struct AMeshAsset : public Asset {
public:
	String File;
	String Name;
	struct SPrimitive {
		String BinaryFile;
		String MaterialFile;
		String Name;
		TVector<String> Textures;
		TVector<FVertex> Vertices;
		TVector<IndexType> Indices;
	};
	TVector<SPrimitive> Primitives;

public:
	AMeshAsset() = default;
	AMeshAsset(const char* file) : File(file) {}

	//load with primitive binaries
	bool Load(const char* file);

	//save without primitives
	bool Save(const char* file);
	bool Save() { return Save(File.c_str()); }

	//load a packed primitive file
	static bool LoadPrimitiveFile(const char* file, TVector<FVertex>& vertices, TVector<IndexType>& indices);

	//export with pack, only execute when import external files
	static bool ExportPrimitiveFile(const char* file, const TVector<FVertex>& vertices, const TVector<IndexType>& indices, EPackMode packMode=EPackMode::NONE);

};
