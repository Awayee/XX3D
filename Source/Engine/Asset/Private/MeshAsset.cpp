#include "Asset/Public/MeshAsset.h"
#include "Core/Public/Json.h"
#include "Math/Public/Math.h"
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
			Normal[0] = Math::PackFloatS1(v.Normal.X);
			Normal[1] = Math::PackFloatS1(v.Normal.Y);
			Normal[2] = Math::PackFloatS1(v.Normal.Z);
			Tangent[0] = Math::PackFloatS1(v.Tangent.X);
			Tangent[1] = Math::PackFloatS1(v.Tangent.Y);
			Tangent[2] = Math::PackFloatS1(v.Tangent.Z);
			memcpy(UV, &v.UV.X, sizeof(float) * 2);
		}
		void Unpack(AssetVertex& v) {
			memcpy(&v.Position.X, Position, sizeof(float) * 3);
			v.Normal = { Math::UnpackFloatS1(Normal[0]), Math::UnpackFloatS1(Normal[1]), Math::UnpackFloatS1(Normal[2]) };
			v.Tangent = { Math::UnpackFloatS1(Tangent[0]), Math::UnpackFloatS1(Tangent[1]), Math::UnpackFloatS1(Tangent[2]) };
			memcpy(&v.UV.X, UV, sizeof(float) * 2);
		}
	};

	struct PackedFTransform {
		uint16 Position[3];
		uint16 Euler[3];
		uint16 Scale[3];
		void Pack(const Math::FTransform& transform) {
			const Math::FVector3 euler = transform.Rotation.ToEuler();
			const Math::FVector3& pos = transform.Position;
			const Math::FVector3& scale = transform.Scale;
			for(uint32 i=0; i<3; ++i) {
				Position[i] = Math::FloatToHalf(pos[i]);
				Scale[i] = Math::FloatToHalf(scale[i]);
				Euler[i] = Math::FloatToHalf(euler[i]);
			}
		}
		void Unpack(Math::FTransform& transform) const {
			Math::FVector3 euler;
			for(uint32 i=0; i<3; ++i) {
				transform.Position[i] = Math::HalfToFloat(Position[i]);
				transform.Scale[i] = Math::HalfToFloat(Scale[i]);
				euler[i] = Math::HalfToFloat(Euler[i]);
			}
			transform.Rotation = Math::FQuaternion::Euler(euler);
		}
	};

	void CalcAABB(TConstArrayView<AssetVertex> vertices, Math::FVector3& outMin, Math::FVector3& outMax) {
		outMin = { FLT_MAX };
		outMax = { -FLT_MAX };
		for(auto& vertex: vertices) {
			outMin = Math::FVector3::Min(vertex.Position, outMin);
			outMax = Math::FVector3::Max(vertex.Position, outMax);
		}
	}

	bool MeshAsset::Load(File::PathStr filePath) {
		Json::Document doc;
		if(!Json::ReadFile(filePath, doc, false)) {
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
				CalcAABB(Primitives[i].Vertices, Primitives[i].AABB.Min, Primitives[i].AABB.Max);
			}
		}
		return true;
	}

	bool MeshAsset::Save(File::PathStr filePath) {
		Json::Document doc(Json::Type::kObjectType);

		//name
		Json::Value nameVal;
		Json::AddStringMember(doc, "Name", Name, doc.GetAllocator());

		//primitives
		Json::Value primitives;
		primitives.SetArray();
		for (uint32 i = 0; i < Primitives.Size(); ++i) {
			Json::Value v(Json::Type::kObjectType);
			Json::AddString(v, "Material", Primitives[i].MaterialFile, doc.GetAllocator());
			Json::AddString(v, "BinaryFile", Primitives[i].BinaryFile, doc.GetAllocator());
			primitives.PushBack(v, doc.GetAllocator());
		}
		doc.AddMember("Primitives", primitives, doc.GetAllocator());
		return Json::WriteFile(filePath, doc, false);
	}

	bool MeshAsset::LoadPrimitiveFile(const char* file, TArray<AssetVertex>& vertices, TArray<IndexType>& indices) {
		PARSE_PROJECT_ASSET(file);
		File::ReadFile f(file, true);
		if(!f.IsOpen()) {
			LOG_WARNING("[MeshAsset::LoadPrimitiveFile] Failed to open file: %s", file);
			return false;
		}

		uint32 vertexCount, indexCount;
		f.Read(&vertexCount, sizeof(uint32));
		f.Read(&indexCount, sizeof(uint32));
		if(0 == vertexCount) {
			LOG_WARNING("[MeshAsset::LoadPrimitiveFile] File is empty: %s", file);
			return false;
		}

		EMeshCompressMode compressMode;
		f.Read(&compressMode, sizeof(EMeshCompressMode));
		uint32 dataByteSize = vertexCount * sizeof(FVertexPack) + indexCount * sizeof(IndexType);
		TArray<char> data(dataByteSize);
		if (compressMode == EMeshCompressMode::NONE) {
			f.Read(data.Data(), dataByteSize);
		}
		else if (compressMode == EMeshCompressMode::LZ4) {
			uint32 compressedByteSize;
			f.Read(&compressedByteSize, sizeof(uint32));
			TArray<char> compressedData(compressedByteSize);
			f.Read(compressedData.Data(), compressedByteSize);
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
			memcpy(indices.Data(), indexPtr, indices.ByteSize());
		}
		return true;
	}

	bool MeshAsset::ExportPrimitiveFile(const char* file, const TArray<AssetVertex>& vertices, const TArray<IndexType>& indices, EMeshCompressMode packMode) {
		if (vertices.Size() == 0) {
			LOG_WARNING("null primitive!");
			return false;
		}
		PARSE_PROJECT_ASSET(file);
		File::WriteFile f(file, true);
		if (!f.IsOpen()) {
			LOG_WARNING("Failed to open file: %s", file);
			return false;
		}
		uint32 vertexCount = vertices.Size();
		uint32 indexCount = indices.Size();
		uint32 dataByteSize = sizeof(FVertexPack) * vertexCount + sizeof(IndexType) * indexCount;
		//header
		f.Write(&vertexCount, sizeof(uint32));
		f.Write(&indexCount, sizeof(uint32));

		//cpy vertices and indices;
		TArray<char> data(dataByteSize);
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
		f.Write(&packMode, sizeof(EMeshCompressMode));

		// no pack
		if (packMode == EMeshCompressMode::NONE) {
			f.Write(data.Data(), data.Size());
		}
		//lz4 pack
		else if (packMode == EMeshCompressMode::LZ4) {
			uint64 compressBound = LZ4_compressBound(dataByteSize);
			TArray<char> compressedData(compressBound);
			uint32 compressedSize = LZ4_compress_default(data.Data(), compressedData.Data(), (int)data.Size(), (int)compressBound);
			compressedData.Resize(compressedSize);
			f.Write(&compressedSize, sizeof(uint32));
			f.Write(compressedData.Data(), compressedData.Size());
		}
		return true;
	}

	bool InstancedMeshAsset::Load(File::PathStr filePath) {
		Json::Document doc;
		if (!Json::ReadFile(filePath, doc, false)) {
			return false;
		}
		if (doc.HasMember("MeshFile")) {
			MeshFile = doc["MeshFile"].GetString();
		}
		if (doc.HasMember("InstanceFile")) {
			InstanceFile = doc["InstanceFile"].GetString();
		}
		return true;
	}

	bool InstancedMeshAsset::Save(File::PathStr filePath) {
		Json::Document doc(Json::Type::kObjectType);
		Json::AddStringMember(doc, "MeshFile", MeshFile, doc.GetAllocator());
		Json::AddStringMember(doc, "InstanceFile", InstanceFile, doc.GetAllocator());
		return Json::WriteFile(filePath, doc, false);
	}

	bool InstancedMeshAsset::LoadInstanceFile(const char* file, TArray<Math::FTransform>& instances) {
		PARSE_PROJECT_ASSET(file);
		instances.Reset();
		if(File::ReadFileWithSize f(file, true); f.IsOpen()) {
			// load meta data size
			uint32 instanceSize;
			f.Read(&instanceSize, sizeof(instanceSize));
			TArray<PackedFTransform> packedTransforms(instanceSize);
			// load and decompress
			const uint32 compressedByteSize = f.ByteSize() - sizeof(instanceSize);
			TArray<char> compressedData(compressedByteSize);
			f.Read(compressedData.Data(), compressedByteSize);
			LZ4_decompress_safe(compressedData.Data(), (char*)packedTransforms.Data(), (int)compressedByteSize, (int)packedTransforms.ByteSize());
			// unpack
			instances.Reserve(instanceSize);
			for(auto& packedTransform: packedTransforms) {
				packedTransform.Unpack(instances.EmplaceBack());
			}
			return true;
		}
		LOG_WARNING("[InstancedMeshAsset::LoadInstanceFile] Failed to load File: %s!", file);
		return false;
	}

	bool InstancedMeshAsset::SaveInstanceFile(const char* file, TConstArrayView<Math::FTransform> instances) {
		if(!instances.Size()) {
			LOG_WARNING("[InstancedMeshAsset::SaveInstanceFile] no instances!");
			return false;
		}
		PARSE_PROJECT_ASSET(file);
		const uint32 instanceSize = instances.Size();
		TArray<char> packedData(sizeof(PackedFTransform) * instanceSize);
		PackedFTransform* packedDataPtr = (PackedFTransform*)packedData.Data();
		for(uint32 i=0; i< instanceSize; ++i) {
			packedDataPtr[i].Pack(instances[i]);
		}
		const uint32 compressBound = LZ4_compressBound((int)packedData.ByteSize());
		TArray<char> compressedData(compressBound);
		const uint32 compressedSize = LZ4_compress_default(packedData.Data(), compressedData.Data(), (int)packedData.Size(), (int)compressBound);
		compressedData.Resize(compressedSize);
		if (File::WriteFile f(file, true); f.IsOpen()) {
			f.Write(&instanceSize, sizeof(instanceSize));
			f.Write(compressedData.Data(), compressedData.ByteSize());
			return true;
		}
		LOG_WARNING("[InstancedMeshAsset::SaveInstanceFile] Failed to load File: %s!", file);
		return false;
	}
}
