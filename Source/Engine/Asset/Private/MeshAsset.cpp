#include "Asset/Public/MeshAsset.h"
#include "Core/Public/Json.h"
#include "Math/Public/MathBase.h"
#include "Core/Public/Log.h"
#include <lz4.h>

namespace Asset {
	struct FVertexPack {
		float Position[3];
		uint8 Normal[3];
		uint8 Tangent[3];
		float UV[2];
		void Pack(const AssetVertex& v) {
			memcpy(Position, &v.Position.X, sizeof(float) * 3);
			Normal[0] = Math::PackFloat01(v.Normal.X);
			Normal[1] = Math::PackFloat01(v.Normal.Y);
			Normal[2] = Math::PackFloat01(v.Normal.Z);
			Tangent[0] = Math::PackFloat01(v.Tangent.X);
			Tangent[1] = Math::PackFloat01(v.Tangent.Y);
			Tangent[2] = Math::PackFloat01(v.Tangent.Z);
			memcpy(UV, &v.UV.X, sizeof(float) * 2);
		}
		void Unpack(AssetVertex& v) {
			memcpy(&v.Position.X, Position, sizeof(float) * 3);
			v.Normal = { Math::UnpackFloat01(Normal[0]), Math::UnpackFloat01(Normal[1]), Math::UnpackFloat01(Normal[2]) };
			v.Tangent = { Math::UnpackFloat01(Tangent[0]), Math::UnpackFloat01(Tangent[1]), Math::UnpackFloat01(Tangent[2]) };
			memcpy(&v.UV.X, UV, sizeof(float) * 2);
		}
	};

	bool MeshAsset::Load(File::RFile& in) {
		Json::Document doc;
		if (!Json::ReadFile(in, doc)) {
			return false;
		}
		if (doc.HasMember("Name")) {
			Name = doc["Name"].GetString();
		}
		if (doc.HasMember("Primitives")) {
			Json::Value::Array primitives = doc["Primitives"].GetArray();
			Primitives.Resize(primitives.Size());
			for (uint32 i = 0; i < primitives.Size(); ++i) {
				Json::Value::Object v = primitives[i].GetObj();
				if (v.HasMember("Material")) {
					Primitives[i].MaterialFile = v["Material"].GetString();
				}
				if (v.HasMember("BinaryFile")) {
					Primitives[i].BinaryFile = v["BinaryFile"].GetString();
				}

				//load primitive binary
				MeshAsset::LoadPrimitiveFile(Primitives[i].BinaryFile.c_str(), Primitives[i].Vertices, Primitives[i].Indices);
			}
		}
		return true;
	}

	bool MeshAsset::Save(File::WFile& out) {
		Json::Document doc;
		doc.SetObject();

		//name
		Json::Value nameVal;
		Json::AddStringMember(doc, "Name", Name, doc.GetAllocator());

		//primitives
		Json::Value primitives;
		primitives.SetArray();
		for (uint32 i = 0; i < Primitives.Size(); ++i) {
			Json::Value v;
			v.SetObject();
			Json::AddString(v, "Material", Primitives[i].MaterialFile, doc.GetAllocator());
			Json::AddString(v, "BinaryFile", Primitives[i].BinaryFile, doc.GetAllocator());
			primitives.PushBack(v, doc.GetAllocator());
		}
		doc.AddMember("Primitives", primitives, doc.GetAllocator());
		return Json::WriteFile(out, doc);
	}
	bool MeshAsset::LoadPrimitiveFile(const char* file, TVector<AssetVertex>& vertices, TVector<IndexType>& indices) {
		PARSE_PROJECT_ASSET(file);
		File::RFile f(file, File::EFileMode::Binary);
		if (!f.is_open()) {
			LOG_INFO("Failed to open file: %s", file);
			return false;
		}
		f.seekg(0);

		uint32 vertexCount, indexCount;
		//header
		f.read(BYTE_PTR(&vertexCount), sizeof(uint32));
		f.read(BYTE_PTR(&indexCount), sizeof(uint32));
		//vertices
		if (vertexCount == 0) {
			f.close();
			return false;
		}

		EMeshCompressMode compressMode;
		f.read(BYTE_PTR(&compressMode), sizeof(EMeshCompressMode));
		uint32 dataByteSize = vertexCount * sizeof(FVertexPack) + indexCount * sizeof(IndexType);
		TVector<char> data(dataByteSize);
		if (compressMode == EMeshCompressMode::NONE) {
			f.read(data.Data(), dataByteSize);
		}
		else if (compressMode == EMeshCompressMode::LZ4) {
			uint32 compressedByteSize;
			f.read(BYTE_PTR(&compressedByteSize), sizeof(uint32));
			TVector<char> compressedData(compressedByteSize);
			f.read(compressedData.Data(), compressedByteSize);
			LZ4_decompress_safe(compressedData.Data(), data.Data(), (int)compressedByteSize, (int)dataByteSize);
		}

		// vertices
		FVertexPack* vertexPtr = reinterpret_cast<FVertexPack*>(data.Data());
		vertices.Resize(vertexCount);
		for (uint32 i = 0; i < vertexCount; ++i) {
			vertexPtr[i].Unpack(vertices[i]);
		}
		//indices
		if (indexCount > 0) {
			indices.Resize(indexCount);
			IndexType* indexPtr = reinterpret_cast<IndexType*>(data.Data() + sizeof(FVertexPack) * vertexCount);
			for (uint32 i = 0; i < indexCount; ++i) {
				indices[i] = indexPtr[i];
			}
		}

		f.close();
		return true;
	}

	bool MeshAsset::ExportPrimitiveFile(const char* file, const TVector<AssetVertex>& vertices, const TVector<IndexType>& indices, EMeshCompressMode packMode) {
		if (vertices.Size() == 0) {
			LOG_INFO("null primitive!");
			return false;
		}

		PARSE_PROJECT_ASSET(file);
		File::WFile f(file, File::EFileMode::Binary | File::EFileMode::Write);
		if (!f.is_open()) {
			LOG_INFO("Failed to open file: %s", file);
			return false;
		}
		uint32 vertexCount = vertices.Size();
		uint32 indexCount = indices.Size();
		uint32 dataByteSize = sizeof(FVertexPack) * vertexCount + sizeof(IndexType) * indexCount;
		//header
		f.write(CBYTE_PTR(&vertexCount), sizeof(uint32));
		f.write(CBYTE_PTR(&indexCount), sizeof(uint32));

		//cpy vertices and indices;
		TVector<char> data(dataByteSize);
		FVertexPack* vertexPtr = reinterpret_cast<FVertexPack*>(data.Data());
		for (uint32 i = 0; i < vertexCount; ++i) {
			vertexPtr[i].Pack(vertices[i]);
		}
		if (indexCount > 0) {
			IndexType* indexPtr = reinterpret_cast<IndexType*>(data.Data() + sizeof(FVertexPack) * vertexCount);
			for (uint32 i = 0; i < indexCount; ++i) {
				indexPtr[i] = indices[i];
			}
		}
		f.write(CBYTE_PTR(&packMode), sizeof(EMeshCompressMode));

		// no pack
		if (packMode == EMeshCompressMode::NONE) {
			f.write(data.Data(), data.Size());
		}
		//lz4 pack
		else if (packMode == EMeshCompressMode::LZ4) {
			uint64 compressBound = LZ4_compressBound(dataByteSize);
			TVector<char> compressedData(compressBound);
			uint32 compressedSize = LZ4_compress_default(data.Data(), compressedData.Data(), (int)data.Size(), (int)compressBound);
			compressedData.Resize(compressedSize);
			f.write(CBYTE_PTR(&compressedSize), sizeof(uint32));
			f.write(compressedData.Data(), compressedData.Size());
		}

		f.close();
		return true;
	}
	
}
