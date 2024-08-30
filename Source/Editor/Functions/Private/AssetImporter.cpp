#include "Functions/Public/AssetImporter.h"
#include "Core/Public/Log.h"
#include "Core/Public/String.h"
#include "Core/Public/Container.h"
#include "Asset/Public/AssetLoader.h"
#include "Asset/Public/MeshAsset.h"
#include "Asset/Public/TextureAsset.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


uint32 GetPrimitiveCount(const tinygltf::Model& model, const tinygltf::Node& node) {
	uint32 count = 0;
	if (node.mesh > -1) {
		count = model.meshes[node.mesh].primitives.size();
	}
	if (!node.children.empty()) {
		for (uint32 i = 0; i < node.children.size(); i++) {
			count += GetPrimitiveCount(model, model.nodes[node.children[i]]);
		}
	}
	return count;
}

void LoadGLTFNode(const tinygltf::Model& model, const tinygltf::Node& node, TArray<Asset::MeshAsset::SPrimitive>& primitives, uint32& index) {
	if (node.mesh > -1) {
		const tinygltf::Mesh& mesh = model.meshes[node.mesh];
		for (uint32 i = 0; i < mesh.primitives.size(); i++) {
			const tinygltf::Primitive& primitive = mesh.primitives[i];
			//vertices
			auto kvPos = primitive.attributes.find("POSITION");
			if (kvPos == primitive.attributes.end()) {
				continue;
			}
			const tinygltf::Accessor& posAccessor = model.accessors[kvPos->second];
			const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
			const float* bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
			uint32 vertexCount = static_cast<uint32>(posAccessor.count);
			int posByteStride = posAccessor.ByteStride(posView) / sizeof(float);

			// normal
			auto kvNormal = primitive.attributes.find("NORMAL");
			const float* bufferNormals = nullptr;
			int normByteStride;
			if (kvNormal != primitive.attributes.end()) {
				const tinygltf::Accessor& normAccessor = model.accessors[kvNormal->second];
				const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
				bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
				normByteStride = normAccessor.ByteStride(normView) / sizeof(float);
			}

			// texcoord
			auto kvTexcoord0 = primitive.attributes.find("TEXCOORD_0");
			const float* bufferTexCoordSet0 = nullptr;
			int uv0ByteStride;
			if (kvTexcoord0 != primitive.attributes.end()) {
				const tinygltf::Accessor& uvAccessor = model.accessors[kvTexcoord0->second];
				const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
				bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
				uv0ByteStride = uvAccessor.ByteStride(uvView) / sizeof(float);
			}

			TArray<Asset::AssetVertex>& vertices = primitives[index].Vertices;
			vertices.Resize(vertexCount);

			for (uint32 v = 0; v < vertexCount; v++) {
				vertices[v].Position.X = bufferPos[v * posByteStride];
				vertices[v].Position.Y = bufferPos[v * posByteStride + 1];
				vertices[v].Position.Z = bufferPos[v * posByteStride + 2];
				vertices[v].Normal.X = bufferNormals[v * normByteStride];
				vertices[v].Normal.Y = bufferNormals[v * normByteStride + 1];
				vertices[v].Normal.Z = bufferNormals[v * normByteStride + 2];
				vertices[v].UV.X = bufferTexCoordSet0[v * uv0ByteStride];
				vertices[v].UV.Y = bufferTexCoordSet0[v * uv0ByteStride + 1];
			}

			//indices
			if (primitive.indices > -1) {
				TArray<uint32>& indices = primitives[index].Indices;
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& bufferIndex = model.buffers[indexView.buffer];
				indices.Resize(indexAccessor.count);
				const void* dataPtr = &(bufferIndex.data[indexAccessor.byteOffset + indexView.byteOffset]);

				if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT) {
					const uint32* buf = static_cast<const uint32*>(dataPtr);
					for (uint32 j = 0; j < indexAccessor.count; j++) {
						indices[j] = buf[j];
					}
				}
				else if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) {
					const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
					for (uint32 j = 0; j < indexAccessor.count; j++) {
						indices[j] = buf[j];
					}
				}
				else if (indexAccessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) {
					const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
					for (uint32 j = 0; j < indexAccessor.count; j++) {
						indices[j] = buf[j];
					}
				}
			}

			// TODO textures
			auto& attr = primitive.attributes;
			if (primitive.material > -1) {
				TArray<std::string>& textureNames = primitives[index].Textures;
				const tinygltf::Material& mat = model.materials[primitive.material];
				if (mat.pbrMetallicRoughness.baseColorTexture.index < model.textures.size()) {
					const tinygltf::Texture& tex = model.textures[mat.pbrMetallicRoughness.baseColorTexture.index];
					const tinygltf::Image& img = model.images[tex.source];
					textureNames.Resize(1);
					uint32 typeIdx = img.mimeType.find('/');
					const char* imgType = &img.mimeType[typeIdx + 1];
					std::string imageFile = img.name + '.' + imgType;
					textureNames[0] = "textures\\";
					textureNames[0].append(imageFile);
				}
			}

			// correct the triangles to counter-clockwise
			auto& primIndices = primitives[index].Indices;
			auto& primVertices = primitives[index].Vertices;
			const uint32 triangleCount = primIndices.Size() / 3;
			for(uint32 triangle=0; triangle<triangleCount; ++triangle) {
				uint32 i0 = triangle * 3;
				uint32 i1 = triangle * 3 + 1;
				uint32 i2 = triangle * 3 + 2;
				uint32 idx0 = primIndices[i0];
				uint32 idx1 = primIndices[i1];
				uint32 idx2 = primIndices[i2];
				auto& v0 = primVertices[idx0].Position;
				auto& v1 = primVertices[idx1].Position;
				auto& v2 = primVertices[idx2].Position;
				auto& n0 = primVertices[idx0].Normal;
				auto& n1 = primVertices[idx1].Normal;
				auto& n2 = primVertices[idx2].Normal;
				auto aveNormal = (n0 + n1 + n2) / 3;
				auto faceNormal = (v2 - v0).Cross(v1 - v0);
				if(faceNormal.Dot(aveNormal) < 0.0f) {
					std::swap<uint32>(primIndices[i1], primIndices[i2]);
				}
			}
			++index;
		}
	}

	if (node.children.size() > 0) {
		for (uint32 i = 0; i < node.children.size(); i++) {
			LoadGLTFNode(model, model.nodes[node.children[i]], primitives, index);
		}
	}
}

uint32 GetPrimitiveCount(const aiScene* aScene, aiNode* aNode) {
	uint32 count = aNode->mNumMeshes;
	for (uint32 i = 0; i < aNode->mNumChildren; i++) {
		count += GetPrimitiveCount(aScene, aNode->mChildren[i]);
	}
	return count;
}

void LoadFbxNode(const aiScene* aScene, aiNode* aNode, TArray<Asset::MeshAsset::SPrimitive>& primitives) {
	for (uint32 i = 0; i < aNode->mNumMeshes; i++) {
		aiMesh* aMesh = aScene->mMeshes[aNode->mMeshes[i]];
		TArray<Asset::AssetVertex>& vertices = primitives[i].Vertices;
		TArray<uint32>& indices = primitives[i].Indices;
		//TArray<std::string>& textures = meshInfos[i].textures;

		// vertices
		vertices.Resize(aMesh->mNumVertices);
		for (uint32 i = 0; i < aMesh->mNumVertices; i++) {
			vertices[i].Position.X = aMesh->mVertices[i].x;
			vertices[i].Position.Y = aMesh->mVertices[i].y;
			vertices[i].Position.Z = aMesh->mVertices[i].z;

			vertices[i].Normal.X = aMesh->mNormals[i].x;
			vertices[i].Normal.Y = aMesh->mNormals[i].y;
			vertices[i].Normal.Z = aMesh->mNormals[i].z;
			if (aMesh->mTextureCoords[0]) {
				vertices[i].UV.X = aMesh->mTextureCoords[0][i].x;
				vertices[i].UV.Y = aMesh->mTextureCoords[0][i].y;
			}
		}

		// indices
		uint32 count = 0;
		for (uint32 i = 0; i < aMesh->mNumFaces; i++) {
			aiFace aFace = aMesh->mFaces[i];
			for (uint32 j = 0; j < aFace.mNumIndices; j++) {
				++count;
			}
		}
		indices.Resize(count);
		count = 0;
		for (uint32 i = 0; i < aMesh->mNumFaces; i++) {
			aiFace aFace = aMesh->mFaces[i];
			for (uint32 j = 0; j < aFace.mNumIndices; j++) {
				indices[count++] = aFace.mIndices[j];
			}
		}



		// correct the triangles to counter-clockwise
		auto& primIndices = primitives[i].Indices;
		auto& primVertices = primitives[i].Vertices;
		const uint32 triangleCount = primIndices.Size() / 3;
		for (uint32 triangle = 0; triangle < triangleCount; ++triangle) {
			uint32 i0 = triangle * 3;
			uint32 i1 = triangle * 3 + 1;
			uint32 i2 = triangle * 3 + 2;
			uint32 idx0 = primIndices[i0];
			uint32 idx1 = primIndices[i1];
			uint32 idx2 = primIndices[i2];
			auto& v0 = primVertices[idx0].Position;
			auto& v1 = primVertices[idx1].Position;
			auto& v2 = primVertices[idx2].Position;
			auto& n0 = primVertices[idx0].Normal;
			auto& n1 = primVertices[idx1].Normal;
			auto& n2 = primVertices[idx2].Normal;
			auto aveNormal = (n0 + n1 + n2) / 3;
			auto faceNormal = (v2 - v0).Cross(v1 - v0);
			if (faceNormal.Dot(aveNormal) < 0.0f) {
				std::swap<uint32>(primIndices[i1], primIndices[i2]);
			}
		}

		// textures TODO
		//if (aMesh->mMaterialIndex >= 0) {
		//	aiMaterial* aMat = aScene->mMaterials[aMesh->mMaterialIndex];
		//	uint32 idx = 0;
		//	aiTextureType types[] = { aiTextureType_DIFFUSE, aiTextureType_NORMALS, aiTextureType_SPECULAR };
		//	for (uint32 j = 0; j < 3; j++) {
		//		for (uint32 i = 0; i < aMat->GetTextureCount(types[j]); i++) { ++idx; }
		//	}
		//	if (idx > 0) {
		//		textures.Resize(idx);
		//		idx = 0;
		//		for (uint32 j = 0; j < 3; j++) {
		//			for (uint32 i = 0; i < aMat->GetTextureCount(types[j]); i++) {
		//				aiString str;
		//				aMat->GetTexture(types[j], i, &str);
		//				textures[idx++] = str.C_Str();
		//			}
		//		}
		//	}
		//}
	}

	for (uint32 i = 0; i < aNode->mNumChildren; i++) {
		LoadFbxNode(aScene, aNode->mChildren[i], primitives);
	}
}

bool ImportGLB(const char* file, Asset::MeshAsset& asset) {
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfContext;
	XString error;
	XString warning;
	XString fullPath(file);
	if (!gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, fullPath)) {
		LOG_WARNING("Failed to load GLTF mesh: %s", file);
		return false;
	}
	const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

	uint32 primitiveCount = 0;
	for (auto& node : scene.nodes) {
		primitiveCount += GetPrimitiveCount(gltfModel, gltfModel.nodes[node]);
	}
	asset.Primitives.Resize(primitiveCount);
	primitiveCount = 0;
	for (uint32 i = 0; i < scene.nodes.size(); i++) {
		LoadGLTFNode(gltfModel, gltfModel.nodes[scene.nodes[i]], asset.Primitives, primitiveCount);
	}
	asset.Name = File::FPath(fullPath).stem().string();
	LOG_INFO("Succeed to load GLTF mesh: %s", file);
	return true;
}

bool ImportFBX(const char* file, Asset::MeshAsset& asset) {
	File::FPath fullPath(file);
	Assimp::Importer importer;
	const aiScene* aScene = importer.ReadFile(fullPath.string().c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);
	if (!aScene || aScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aScene->mRootNode) {
		LOG_DEBUG("ASSIMP ERROR: %s", importer.GetErrorString());
		return false;
	}
	// 先获取总面数
	uint32 primitiveCount = GetPrimitiveCount(aScene, aScene->mRootNode);
	asset.Primitives.Resize(primitiveCount);
	LoadFbxNode(aScene, aScene->mRootNode, asset.Primitives);
	asset.Name = File::FPath(fullPath).stem().string();
	LOG_INFO("Succeed to load GLTF mesh: %s", file);
	return true;
}

bool ImportMeshAsset(const XString& srcFile, const XString& dstFile) {
	Asset::MeshAsset asset;
	auto saveFile = dstFile;
	bool r = true;
	if (saveFile.empty()) {
		File::FPath relativePath = File::RelativePath(File::FPath(srcFile), Asset::AssetLoader::AssetPath());
		relativePath.replace_extension(".mesh");
		saveFile = relativePath.string();
	}
	if (StrEndsWith(srcFile.c_str(), ".glb")) {
		r = ImportGLB(srcFile.c_str(), asset);
	}
	else if (StrEndsWith(srcFile.c_str(), ".fbx")) {
		r = ImportFBX(srcFile.c_str(), asset);
	}
	if(!r) {
		LOG_WARNING("Can not import mesh %s", srcFile.c_str());
		return r;
	}

	// save

	const File::FPath FullPath(saveFile.c_str());

	//write primitives
	TUnorderedSet<XString> usedNames;
	for (uint32 i = 0; i < asset.Primitives.Size(); ++i) {
		auto& primitive = asset.Primitives[i];
		//generate name if empty
		if (primitive.Name.empty() || usedNames.find(primitive.Name) != usedNames.end()) {
			primitive.Name = FullPath.stem().filename().string() + ToString<uint32>(i);
		}

		File::FPath parentPath = FullPath.parent_path();
		parentPath.append(primitive.Name);
		parentPath.replace_extension(".primitive");
		XString binaryFile = parentPath.string();
		std::replace(binaryFile.begin(), binaryFile.end(), '\\', '/');

		r |= Asset::MeshAsset::ExportPrimitiveFile(binaryFile.c_str(), primitive.Vertices, primitive.Indices, Asset::EMeshCompressMode::NONE);
		primitive.BinaryFile.swap(binaryFile);
	}
	r |= Asset::AssetLoader::SaveProjectAsset(&asset, saveFile.c_str());
	return r;
}

struct StbiImageScope {
	uint8* Data{ nullptr };
	uint8* Load(const char* filename, int* width, int* height, int* channels, int desiredChannels) {
		Data = stbi_load(filename, width, height, channels, desiredChannels);
		return Data;
	}	
	~StbiImageScope() {
		if(Data) {
			stbi_image_free(Data);
		}
	}
};

bool ImportTexture2DAsset(const XString& srcFile, const XString& dstFile) {
	XString saveFile = dstFile;
	if (saveFile.empty()) {
		File::FPath relativePath = File::RelativePath(File::FPath(srcFile), Asset::AssetLoader::AssetPath());
		relativePath.replace_extension(".texture");
		saveFile = relativePath.string();
	}

	int width, height, channels;
	constexpr int desiredChannels = STBI_rgb_alpha;
	uint8* pixels = stbi_load(srcFile.c_str(), &width, &height, &channels, desiredChannels);
	if (!pixels) {
		LOG_INFO("loaded image is empty!");
		return false;
	}

	Asset::TextureAsset asset;
	asset.Width = width;
	asset.Height = height;
	asset.Type = Asset::ETextureAssetType::RGBA8_2D;
	uint32 byteSize = width * height * desiredChannels;
	asset.Pixels.Resize(byteSize);
	memcpy(asset.Pixels.Data(), pixels, byteSize);
	stbi_image_free(pixels);

	// save
	return Asset::AssetLoader::SaveProjectAsset(&asset, saveFile.c_str());
}

bool ImportTextureCubeAsset(TConstArrayView<XString> srcFiles, const XString& dstFile) {
	if(srcFiles.Size() < 6) {
		LOG_WARNING("[ImportTextureCubeAsset] src files error!");
		return false;
	}
	XString saveFile = dstFile;
	if(saveFile.empty()) {
		File::FPath relativePath = File::RelativePath(File::FPath(srcFiles[0]), Asset::AssetLoader::AssetPath());
		relativePath.replace_extension(".texture");
		saveFile = relativePath.string();
	}
	Asset::TextureAsset asset;
	asset.Type = Asset::ETextureAssetType::RGBA8_Cube;
	bool flag = false;
	uint32 sliceByteSize = 0;
	for(uint32 i=0; i<6; ++i) {
		int width, height, channels;
		constexpr int desiredChannels = STBI_rgb_alpha;
		StbiImageScope s;
		uint8* pixels = s.Load(srcFiles[i].c_str(), &width, &height, &channels, desiredChannels);
		if (!pixels) {
			LOG_WARNING("[ImportTextureCubeAsset]loaded image is empty!");
			return false;
		}
		if(!flag) {
			asset.Width = width;
			asset.Height = height;
			sliceByteSize = width * height * 4;
			flag = true;
		}
		else if(asset.Width != width || asset.Height != height) {
			LOG_WARNING("[ImportTextureCubeAsset]Elements size are not equal!");
			return false;
		}
		memcpy(asset.Pixels.Data() + i * sliceByteSize, pixels, sliceByteSize);
	}
	return Asset::AssetLoader::SaveProjectAsset(&asset, dstFile.c_str());
}
