#include "Asset/Public/MeshAsset.h"
#include "Core/Public/Json.h"
#include "Math/Public/MathBase.h"
#include "Core/Public/macro.h"
#include <lz4.h>

struct FVertexPack {
	float Position[3];
	uint8 Normal[3];
	uint8 Tangent[3];
	float UV[2];
	void Pack(const FVertex& v) {
		memcpy(Position, &v.Position.x, sizeof(float) * 3);
		Normal[0] = Math::PackFloat01(v.Normal.x);
		Normal[1] = Math::PackFloat01(v.Normal.y);
		Normal[2] = Math::PackFloat01(v.Normal.z);
		Tangent[0] = Math::PackFloat01(v.Tangent.x);
		Tangent[1] = Math::PackFloat01(v.Tangent.y);
		Tangent[2] = Math::PackFloat01(v.Tangent.z);
		memcpy(UV, &v.UV.x, sizeof(float) * 2);
	}
	void Unpack(FVertex& v) {
		memcpy(&v.Position.x, Position, sizeof(float) * 3);
		v.Normal = { Math::UnpackFloat01(Normal[0]), Math::UnpackFloat01(Normal[1]), Math::UnpackFloat01(Normal[2]) };
		v.Tangent = { Math::UnpackFloat01(Tangent[0]), Math::UnpackFloat01(Tangent[1]), Math::UnpackFloat01(Tangent[2]) };
		memcpy(&v.UV.x, UV, sizeof(float) * 2);
	}
};

bool AMeshAsset::Load(const char* file) {
	PARSE_PROJECT_ASSET(file);
	Json::Document doc;
	if(!Json::ReadFile(file, doc)) {
		return false;
	}
	if(doc.HasMember("Name")) {
		Name = doc["Name"].GetString();
	}
	if(doc.HasMember("Primitives")) {
		Json::Value::Array primitives = doc["Primitives"].GetArray();
		Primitives.resize(primitives.Size());
		for(uint32 i=0; i<primitives.Size(); ++i) {
			Json::Value::Object v = primitives[i].GetObj();
			if(v.HasMember("Material")) {
				Primitives[i].MaterialFile = v["Material"].GetString();
			}
			if(v.HasMember("BinaryFile")) {
				Primitives[i].BinaryFile = v["BinaryFile"].GetString();
			}

			//load primitive binary
			AMeshAsset::LoadPrimitiveFile(Primitives[i].BinaryFile.c_str(), Primitives[i].Vertices, Primitives[i].Indices);
		}
	}
	File = file;
	return true;
}

bool AMeshAsset::Save(const char* file) {

	PARSE_PROJECT_ASSET(file);

	Json::Document doc;
	doc.SetObject();

	//name
	Json::Value nameVal;
	Json::AddStringMember(doc, "Name", Name, doc.GetAllocator());

	//primitives
	Json::Value primitives;
	primitives.SetArray();
	for(uint32 i=0; i<Primitives.size(); ++i) {
		Json::Value v;
		v.SetObject();
		Json::AddString(v, "Material", Primitives[i].MaterialFile, doc.GetAllocator());
		Json::AddString(v, "BinaryFile", Primitives[i].BinaryFile, doc.GetAllocator());
		primitives.PushBack(v, doc.GetAllocator());
	}
	doc.AddMember("Primitives", primitives, doc.GetAllocator());
	return Json::WriteFile(file, doc);
}
bool AMeshAsset::LoadPrimitiveFile(const char* file, TVector<FVertex>& vertices, TVector<IndexType>& indices) {
	PARSE_PROJECT_ASSET(file);
	File::Read f(file, std::ios::binary);
	if(!f.is_open()) {
		LOG("Failed to open file: %s", file);
		return false;
	}
	f.seekg(0);

	uint32 vertexCount, indexCount;
	uint32 packedByteSize;
	//header
	f.read(reinterpret_cast<char*>(&vertexCount), sizeof(uint32));
	f.read(reinterpret_cast<char*>(&indexCount), sizeof(uint32));
	f.read(reinterpret_cast<char*>(&packedByteSize), sizeof(uint32));
	//vertices
	if(vertexCount == 0) {
		f.close();
		return false;
	}

	EPackMode packMode;
	f.read(reinterpret_cast<char*>(&packMode), sizeof(EPackMode));
	uint32 dataByteSize = vertexCount * sizeof(FVertexPack) + indexCount * sizeof(IndexType);
	TVector<char> data(dataByteSize);
	if(packMode == EPackMode::NONE) {
		f.read(data.data(), dataByteSize);
	}
	else if(packMode == EPackMode::LZ4) {
		TVector<char> packedData(packedByteSize);
		f.read(packedData.data(), packedByteSize);
		LZ4_decompress_safe(packedData.data(), data.data(), (int)packedByteSize, (int)dataByteSize);
	}

	// vertices
	FVertexPack* vertexPtr = reinterpret_cast<FVertexPack*>(data.data());
	vertices.resize(vertexCount);
	for(uint32 i=0; i< vertexCount; ++i) {
		vertexPtr[i].Unpack(vertices[i]);
	}
	//indices
	if(indexCount > 0) {
		indices.resize(indexCount);
		IndexType* indexPtr = reinterpret_cast<IndexType*>(data.data() + sizeof(FVertexPack) * vertexCount);
		for(uint32 i=0; i<indexCount;++i) {
			indices[i] = indexPtr[i];
		}
	}

	f.close();
	return true;
}

bool AMeshAsset::ExportPrimitiveFile(const char* file, const TVector<FVertex>& vertices, const TVector<IndexType>& indices, EPackMode packMode) {
	if(vertices.size() == 0) {
		LOG("null primitive!");
		return false;
	}

	PARSE_PROJECT_ASSET(file);
	File::Write f(file, std::ios::binary | std::ios::out);
	if(!f.is_open()) {
		LOG("Failed to open file: %s", file);
		return false;
	}
	uint32 vertexCount = vertices.size();
	uint32 indexCount = indices.size();
	uint32 dataByteSize = sizeof(FVertexPack) * vertexCount + sizeof(IndexType) * indexCount;
	//header
	f.write(reinterpret_cast<char*>(&vertexCount), sizeof(uint32));
	f.write(reinterpret_cast<char*>(&indexCount), sizeof(uint32));
	f.write(reinterpret_cast<char*>(&dataByteSize), sizeof(uint32));

	//cpy vertices and indices;
	TVector<char> data(dataByteSize);
	FVertexPack* vertexPtr = reinterpret_cast<FVertexPack*>(data.data());
	for(uint32 i=0; i<vertexCount; ++i) {
		vertexPtr[i].Pack(vertices[i]);
	}
	if(indexCount > 0) {
		IndexType* indexPtr = reinterpret_cast<IndexType*>(data.data() + sizeof(FVertexPack) * vertexCount);
		for(uint32 i=0; i<indexCount; ++i) {
			indexPtr[i] = indices[i];
		}
	}
	f.write(reinterpret_cast<char*>(&packMode), sizeof(EPackMode));

	// no pack
	if(packMode == EPackMode::NONE) {
		f.write(data.data(), data.size());
	}
	//lz4 pack
	else if(packMode == EPackMode::LZ4) {
		uint64 compressBound = LZ4_compressBound(dataByteSize);
		TVector<char> packedData(compressBound);
		int compressedSize = LZ4_compress_default(data.data(), packedData.data(), (int)data.size(), (int)compressBound);
		packedData.resize(compressedSize);
		f.write(packedData.data(), packedData.size());
	}

	f.close();
	return true;
}
