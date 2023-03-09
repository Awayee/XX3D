#include "MeshUtil.h"
#include "Core/Public/String.h"
#include "Core/Public/File.h"
#include "Core/Public/macro.h"
#include "Resource/Public/AssetData.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

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

void LoadGLTFNode(const tinygltf::Model& model, const tinygltf::Node& node, TVector<SPrimitiveData>& primitives, uint32& index) {
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

			TVector<FVertex>& vertices = primitives[index].Vertices;
			vertices.resize(vertexCount);

			for (uint32 v = 0; v < vertexCount; v++) {
				vertices[v].Position.x = bufferPos[v * posByteStride];
				vertices[v].Position.y = bufferPos[v * posByteStride + 1];
				vertices[v].Position.z = bufferPos[v * posByteStride + 2];
				vertices[v].Normal.x = bufferNormals[v * normByteStride];
				vertices[v].Normal.y = bufferNormals[v * normByteStride + 1];
				vertices[v].Normal.z = bufferNormals[v * normByteStride + 2];
				vertices[v].UV.x = bufferTexCoordSet0[v * uv0ByteStride];
				vertices[v].UV.y = bufferTexCoordSet0[v * uv0ByteStride + 1];
			}

			//indices
			if (primitive.indices > -1) {
				TVector<uint32>& indices = primitives[index].Indices;
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& bufferIndex = model.buffers[indexView.buffer];
				indices.resize(indexAccessor.count);
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
			if (primitive.material > -1) {
				TVector<std::string>& textureNames = primitives[index].Textures;
				const tinygltf::Material& mat = model.materials[primitive.material];
				if(mat.pbrMetallicRoughness.baseColorTexture.index < model.textures.size()) {
					const tinygltf::Texture& tex = model.textures[mat.pbrMetallicRoughness.baseColorTexture.index];
					const tinygltf::Image& img = model.images[tex.source];
					textureNames.resize(1);
					uint32 typeIdx = img.mimeType.find('/');
					const char* imgType = &img.mimeType[typeIdx + 1];
					std::string imageFile = img.name + '.' + imgType;
					textureNames[0] = "textures\\";
					textureNames[0].append(imageFile);
				}
			}
			++index;
			auto& info = primitives[index - 1];
			auto& verts = info.Vertices;
		}
	}

	if (node.children.size() > 0) {
		for (uint32 i = 0; i < node.children.size(); i++) {
			LoadGLTFNode(model, model.nodes[node.children[i]], primitives, index);
		}
	}
}

// TODO run in another thread
bool LoadMeshFromGLTF(const char* file, TVector<SPrimitiveData>& primitives)
{
	String fullPath = file;
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfContext;
	String error;
	String warning;
	if(!gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning, fullPath)) {
		LOG("Failed to load GLTF mesh: %s", file);
		return false;
	}
	const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

	uint32 primitiveCount = 0;
	for(auto& node: scene.nodes) {
		primitiveCount += GetPrimitiveCount(gltfModel, gltfModel.nodes[node]);
	}
	primitives.resize(primitiveCount);
	primitiveCount = 0;
	for (uint32 i = 0; i < scene.nodes.size(); i++) {
		LoadGLTFNode(gltfModel, gltfModel.nodes[scene.nodes[i]], primitives, primitiveCount);
	}
	return true;
}

uint32 GetPrimitiveCount(const aiScene* aScene, aiNode* aNode) {
	uint32 count = aNode->mNumMeshes;
	for (uint32 i = 0; i < aNode->mNumChildren; i++) {
		count += GetPrimitiveCount(aScene, aNode->mChildren[i]);
	}
	return count;
}

void LoadFbxNode(const aiScene* aScene, aiNode* aNode, TVector<SPrimitiveData>& meshInfos) {
	for (uint32 i = 0; i < aNode->mNumMeshes; i++) {
		aiMesh* aMesh = aScene->mMeshes[aNode->mMeshes[i]];
		TVector<FVertex>& vertices = meshInfos[i].Vertices;
		TVector<uint32>& indices = meshInfos[i].Indices;
		//TVector<std::string>& textures = meshInfos[i].textures;

		// vertices
		vertices.resize(aMesh->mNumVertices);
		for (uint32 i = 0; i < aMesh->mNumVertices; i++) {
			vertices[i].Position.x = aMesh->mVertices[i].x;
			vertices[i].Position.y = aMesh->mVertices[i].y;
			vertices[i].Position.z = aMesh->mVertices[i].z;

			vertices[i].Normal.x = aMesh->mNormals[i].x;
			vertices[i].Normal.y = aMesh->mNormals[i].y;
			vertices[i].Normal.z = aMesh->mNormals[i].z;
			if (aMesh->mTextureCoords[0]) {
				vertices[i].UV.x = aMesh->mTextureCoords[0][i].x;
				vertices[i].UV.y = aMesh->mTextureCoords[0][i].y;
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
		indices.resize(count);
		count = 0;
		for (uint32 i = 0; i < aMesh->mNumFaces; i++) {
			aiFace aFace = aMesh->mFaces[i];
			for (uint32 j = 0; j < aFace.mNumIndices; j++) {
				indices[count++] = aFace.mIndices[j];
			}
		}

		// textures TODO
		//if (aMesh->mMaterialIndex >= 0) {
		//	// 只读取需要的纹理
		//	aiMaterial* aMat = aScene->mMaterials[aMesh->mMaterialIndex];
		//	uint32 idx = 0;
		//	aiTextureType types[] = { aiTextureType_DIFFUSE, aiTextureType_NORMALS, aiTextureType_SPECULAR };
		//	for (uint32 j = 0; j < 3; j++) {
		//		for (uint32 i = 0; i < aMat->GetTextureCount(types[j]); i++) { ++idx; }
		//	}
		//	if (idx > 0) {
		//		textures.resize(idx);
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
		LoadFbxNode(aScene, aNode->mChildren[i], meshInfos);
	}
}

bool LoadMeshFromFBX(const char* file, TVector<SPrimitiveData>& primitives) {
	String fullPath = file;
	Assimp::Importer importer;
	const aiScene* aScene = importer.ReadFile(fullPath, aiProcess_Triangulate | aiProcess_FlipUVs);
	if (!aScene || aScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aScene->mRootNode) {
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return false;
	}
	// 先获取总面数
	uint32 primitiveCount = GetPrimitiveCount(aScene, aScene->mRootNode);
	primitives.resize(primitiveCount);
	LoadFbxNode(aScene, aScene->mRootNode, primitives);
	return true;
}
