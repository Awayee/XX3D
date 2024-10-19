#include "Functions/Public/AssetImporter.h"
#include "Core/Public/Log.h"
#include "Core/Public/String.h"
#include "Core/Public/Container.h"
#include "Asset/Public/AssetLoader.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE2_IMPLEMENTATION
#include <stb_image_resize2.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Math/Public/Math.h"

struct MeshPrimitiveTemp {
	TArray<Asset::AssetVertex> Vertices;
	TArray<Asset::IndexType> Indices;
	XString Name;
	TArray<XString> Textures;
};

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

void LoadGLTFNode(const tinygltf::Model& model, const tinygltf::Node& node, TArray<MeshPrimitiveTemp>& primitives, uint32& index, float uniformScale) {
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
				vertices[v].Position.X = uniformScale * bufferPos[v * posByteStride];
				vertices[v].Position.Y = uniformScale * bufferPos[v * posByteStride + 1];
				vertices[v].Position.Z = uniformScale * bufferPos[v * posByteStride + 2];
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
				auto& textureNames = primitives[index].Textures;
				const tinygltf::Material& mat = model.materials[primitive.material];
				if (mat.pbrMetallicRoughness.baseColorTexture.index < model.textures.size()) {
					const tinygltf::Texture& tex = model.textures[mat.pbrMetallicRoughness.baseColorTexture.index];
					const tinygltf::Image& img = model.images[tex.source];
					textureNames.Resize(1);
					uint32 typeIdx = img.mimeType.find('/');
					const char* imgType = &img.mimeType[typeIdx + 1];
					XString imageFile = img.name + '.' + imgType;
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
			LoadGLTFNode(model, model.nodes[node.children[i]], primitives, index, uniformScale);
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

void LoadFbxNode(const aiScene* aScene, aiNode* aNode, TArray<MeshPrimitiveTemp>& primitives, float uniformScale) {
	for (uint32 i = 0; i < aNode->mNumMeshes; i++) {
		aiMesh* aMesh = aScene->mMeshes[aNode->mMeshes[i]];
		TArray<Asset::AssetVertex>& vertices = primitives[i].Vertices;
		TArray<uint32>& indices = primitives[i].Indices;
		//TArray<std::string>& textures = meshInfos[i].textures;

		if(0 == aMesh->mNumVertices || 0 == aMesh->mNumFaces) {
			LOG_WARNING("[LoadFbxNode] Num of vertex or num of index is 0!");
		}
		// vertices
		vertices.Resize(aMesh->mNumVertices);
		for (uint32 i = 0; i < aMesh->mNumVertices; i++) {
			vertices[i].Position.X = uniformScale * aMesh->mVertices[i].x;
			vertices[i].Position.Y = uniformScale * aMesh->mVertices[i].y;
			vertices[i].Position.Z = uniformScale * aMesh->mVertices[i].z;
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
		LoadFbxNode(aScene, aNode->mChildren[i], primitives, uniformScale);
	}
}

// TODO gltf internal textures
bool LoadImageDataFunction(tinygltf::Image*, const int, std::string*,
                                      std::string*, int, int,
                                      const unsigned char*, int,
                                      void* user_pointer) {
	return true;
}

bool ImportGLTF(const char* file, TArray<MeshPrimitiveTemp>& primitives, bool isBinary, float uniformScale) {
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfContext;
	gltfContext.SetImageLoader(LoadImageDataFunction, nullptr);
	XString error;
	XString warning;
	XString fullPath(file);
	if(isBinary) {
		if (!gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, fullPath)) {
			LOG_WARNING("Failed to load binary GLTF mesh: %s, Error: %s", file, error.c_str());
			return false;
		}
	}
	else if (!gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, fullPath)) {
		LOG_WARNING("Failed to load GLTF mesh: %s, Error: %s", file, error.c_str());
		return false;
	}
	const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

	uint32 primitiveCount = 0;
	for (auto& node : scene.nodes) {
		primitiveCount += GetPrimitiveCount(gltfModel, gltfModel.nodes[node]);
	}
	primitives.Resize(primitiveCount);
	primitiveCount = 0;
	for (uint32 i = 0; i < scene.nodes.size(); i++) {
		LoadGLTFNode(gltfModel, gltfModel.nodes[scene.nodes[i]], primitives, primitiveCount, uniformScale);
	}
	LOG_INFO("Succeed to load GLTF mesh: %s", file);
	return true;
}

bool ImportFBX(const char* file, TArray<MeshPrimitiveTemp>& primitives, float uniformScale) {
	File::FPath fullPath(file);
	Assimp::Importer importer;
	const aiScene* aScene = importer.ReadFile(fullPath.string().c_str(), aiProcess_Triangulate | aiProcess_FlipUVs);
	if (!aScene || aScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aScene->mRootNode) {
		LOG_DEBUG("ASSIMP ERROR: %s", importer.GetErrorString());
		return false;
	}
	uint32 primitiveCount = GetPrimitiveCount(aScene, aScene->mRootNode);
	primitives.Resize(primitiveCount);
	LoadFbxNode(aScene, aScene->mRootNode, primitives, uniformScale);
	LOG_INFO("Succeed to load GLTF mesh: %s", file);
	return true;
}

bool ImportMeshAsset(const XString& srcFile, const XString& dstFile, float uniformScale) {
	Asset::MeshAsset asset;
	TArray<MeshPrimitiveTemp> primitives;
	auto saveFile = dstFile;
	bool r = false;
	if (saveFile.empty()) {
		File::FPath relativePath = Asset::AssetLoader::GetRelativePath(srcFile.c_str());
		relativePath.replace_extension(".mesh");
		saveFile = relativePath.string();
	}
	if (StrEndsWith(srcFile.c_str(), ".glb")) {
		r = ImportGLTF(srcFile.c_str(), primitives, true, uniformScale);
	}
	else if (StrEndsWith(srcFile.c_str(), ".gltf")) {
		r = ImportGLTF(srcFile.c_str(), primitives, false, uniformScale);
	}
	else if (StrEndsWith(srcFile.c_str(), ".fbx")) {
		r = ImportFBX(srcFile.c_str(), primitives, uniformScale);
	}
	else {
		LOG_WARNING("[ImportMeshAsset] Unsupported file type: %s", srcFile.c_str());
		return false;
	}
	if(!r) {
		LOG_WARNING("[ImportMeshAsset] Can not import mesh %s", srcFile.c_str());
		return r;
	}
	// save primitives
	const File::FPath savePath(saveFile.c_str());
	TSet<XString> usedNames;
	asset.Primitives.Resize(primitives.Size());
	for (uint32 i = 0; i < asset.Primitives.Size(); ++i) {
		auto& primitive = asset.Primitives[i];
		auto& srcPrimitive = primitives[i];
		//generate name if empty)
		if (srcPrimitive.Name.empty() || usedNames.find(primitive.Name) != usedNames.end()) {
			primitive.Name = savePath.stem().filename().string() + ToString<uint32>(i);
		}
		XString primitiveFile = savePath.parent_path().append(primitive.Name).replace_extension(".primitive").string();
		std::replace(primitiveFile.begin(), primitiveFile.end(), '\\', '/');
		Asset::PrimitiveAsset primitiveAsset;
		primitiveAsset.Vertices.Swap(srcPrimitive.Vertices);
		primitiveAsset.Indices.Swap(srcPrimitive.Indices);
		Asset::AssetLoader::SaveProjectAsset(&primitiveAsset, primitiveFile.c_str());
		primitive.BinaryFile.swap(primitiveFile);
	}
	// save mesh
	r |= Asset::AssetLoader::SaveProjectAsset(&asset, saveFile.c_str());
	return r;
}

struct StbiImageScope {
	uint8* Data{ nullptr };
	StbiImageScope(const char* filename, int* width, int* height, int*channels, int desiredChannels) {
		Data = stbi_load(filename, width, height, channels, desiredChannels);
	}
	~StbiImageScope() {
		if(Data) {
			stbi_image_free(Data);
		}
	}
};

bool ImportTexture2DAsset(const XString& srcFile, const XString& dstFile, int downsize, Asset::ETextureCompressMode compressMode) {
	XString saveFile = dstFile;
	if (saveFile.empty()) {
		File::FPath relativePath = Asset::AssetLoader::GetRelativePath(srcFile.c_str());
		relativePath.replace_extension(".texture");
		saveFile = relativePath.string();
	}

	int width, height, channels;
	constexpr int desiredChannels = STBI_rgb_alpha;
	StbiImageScope s(srcFile.c_str(), &width, &height, &channels, desiredChannels);
	if (!s.Data) {
		LOG_INFO("[ImportTexture2DAsset] Image is empty!");
		return false;
	}

	Asset::TextureAsset asset;
	asset.Type = Asset::ETextureAssetType::RGBA8Srgb_2D;
	asset.CompressMode = compressMode;
	// downsize
	if(downsize > 1) {
		const int downsizedWidth = Math::Max(width / downsize, 1);
		const int downsizeHeight = Math::Max(height / downsize, 1);
		const uint32 downsizedByteSize = downsizedWidth * downsizeHeight * desiredChannels;
		asset.Pixels.Resize(downsizedByteSize);
		stbir_resize_uint8_linear(s.Data, width, height, 0, 
			asset.Pixels.Data(), downsizedWidth, downsizeHeight, 0, stbir_pixel_layout::STBIR_4CHANNEL);
		asset.Width = downsizedWidth;
		asset.Height = downsizeHeight;
	}
	else {
		asset.Width = width;
		asset.Height = height;
		uint32 byteSize = width * height * desiredChannels;
		asset.Pixels.Resize(byteSize);
		memcpy(asset.Pixels.Data(), s.Data, byteSize);
	}

	// save
	return Asset::AssetLoader::SaveProjectAsset(&asset, saveFile.c_str());
}

bool ImportTextureCubeAsset(TConstArrayView<XString> srcFiles, const XString& dstFile, int downsize, Asset::ETextureCompressMode compressMode) {
	if(srcFiles.Size() < 6) {
		LOG_WARNING("[ImportTextureCubeAsset] Input files error!");
		return false;
	}
	XString saveFile = dstFile;
	if(saveFile.empty()) {
		File::FPath relativePath = Asset::AssetLoader::GetRelativePath(srcFiles[0].c_str());
		relativePath.replace_extension(".texture");
		saveFile = relativePath.string();
	}
	Asset::TextureAsset asset;
	asset.Type = Asset::ETextureAssetType::RGBA8Srgb_Cube;
	asset.CompressMode = compressMode;
	// get the first slice for size
	bool flag = false;
	uint32 sliceByteSize = 0;
	for(uint32 i=0; i<6; ++i) {
		int srcWidth, srcHeight, channels;
		constexpr int desiredChannels = STBI_rgb_alpha;
		StbiImageScope s(srcFiles[i].c_str(), &srcWidth, &srcHeight, &channels, desiredChannels);
		if (!s.Data) {
			LOG_WARNING("[ImportTextureCubeAsset] Image is empty!");
			return false;
		}
		const int trueWidth = downsize > 1 ? Math::Max(srcWidth / downsize, 1) : srcWidth;
		const int trueHeight = downsize > 1 ? Math::Max(srcHeight / downsize, 1) : srcHeight;
		if(!flag) {
			asset.Width = trueWidth;
			asset.Height = trueHeight;
			sliceByteSize = trueWidth * trueHeight * desiredChannels;
			asset.Pixels.Resize(sliceByteSize * 6);
			flag = true;
		}
		else if(asset.Width != trueWidth || asset.Height != trueHeight) {
			LOG_WARNING("[ImportTextureCubeAsset] Elements are not equal!");
			return false;
		}
		uint8* dstData = asset.Pixels.Data() + i * sliceByteSize;
		// downsize
		if(downsize > 1) {
			stbir_resize_uint8_linear(s.Data, srcWidth, srcHeight, 0,
				dstData, trueWidth, trueHeight, 0, stbir_pixel_layout::STBIR_4CHANNEL);
		}
		else {
			memcpy(dstData, s.Data, sliceByteSize);
		}
	}
	return Asset::AssetLoader::SaveProjectAsset(&asset, saveFile.c_str());
}
