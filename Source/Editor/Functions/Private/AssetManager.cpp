#include "Functions/Public/AssetManager.h"
#include "Core/Public/File.h"
#include "Core/Public/Log.h"
#include "Core/Public/String.h"
#include "Functions/Public/AssetImporter.h"
#include "Math/Public/Math.h"


namespace {
	using namespace Math;
	inline Math::FVector3 CalcTangentVec(FVector3 pos0, FVector3 pos1, FVector3 pos2, FVector2 uv0, FVector2 uv1, FVector2 uv2) {
		FVector3 edge0 = pos1 - pos0;
		FVector3 edge1 = pos2 - pos0;
		FVector2 deltaUV0 = uv1 - uv0;
		FVector2 deltaUV1 = uv2 - uv0;
		float f = 1.0f / (deltaUV0.X * deltaUV1.Y - deltaUV1.X * deltaUV0.Y);
		return FVector3{
			f * (deltaUV1.Y * edge0.X - deltaUV0.Y * edge1.X),
			f * (deltaUV1.Y * edge0.Y - deltaUV0.Y * edge1.Y),
			f * (deltaUV1.Y * edge0.Z - deltaUV0.Y * edge1.Z)
		};
	}


	void BakeEngineAssets() {
		{
			TArray<Asset::AssetVertex> vertices{
				{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {},  {0.0f,  0.0f}},
				{{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {},  {1.0f,  0.0f}},
				{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {},  {1.0f,  1.0f}},
				{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {},  {1.0f,  1.0f}},
				{{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {},  {0.0f,  1.0f}},
				{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {},  {0.0f,  0.0f}},

				{{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {0.0f,  0.0f}},
				{{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {1.0f,  0.0f}},
				{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {1.0f,  1.0f}},
				{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {1.0f,  1.0f}},
				{{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {0.0f,  1.0f}},
				{{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {0.0f,  0.0f}},

				{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {1.0f,  1.0f}},
				{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {0.0f,  0.0f}},
				{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {1.0f,  0.0f}},

				{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {1.0f,  1.0f}},
				{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {0.0f,  0.0f}},
				{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {1.0f,  0.0f}},

				{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {1.0f,  1.0f}},
				{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {0.0f,  0.0f}},
				{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {0.0f,  1.0f}},

				{{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {},  {1.0f,  1.0f}},
				{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {},  {0.0f,  0.0f}},
				{{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {},  {0.0f,  1.0f}}
			};
			// calc tangent
			const uint32 triangleCount = vertices.Size() / 3;
			for (uint32 i = 0; i < triangleCount; ++i) {
				const uint32 i0 = i * 3;
				const uint32 i1 = i * 3 + 1;
				const uint32 i2 = i * 3 + 2;
				FVector3 tangent = CalcTangentVec(vertices[i0].Position, vertices[i1].Position, vertices[i2].Position,
					vertices[i0].UV, vertices[i1].UV, vertices[i2].UV);
				vertices[i0].Tangent = vertices[i1].Tangent = vertices[i1].Tangent = tangent;
			}
			// fill indices
			TArray<uint32> indices(vertices.Size());
			for (uint32 i = 0; i < indices.Size(); ++i) {
				indices[i] = i;
			}

			Asset::MeshAsset a;
			a.Name = "Cube";
			auto& primitive = a.Primitives.EmplaceBack();
			primitive.BinaryFile = "Meshes/Cube.primitive";
			primitive.Vertices.Swap(vertices);
			primitive.Indices.Swap(indices);
			a.ExportPrimitiveFile(primitive.BinaryFile.c_str(), primitive.Vertices, primitive.Indices);
			Asset::AssetLoader::SaveProjectAsset(&a, "Meshes/Cube.mesh");
		}

		{
			// sphere
			constexpr uint32 division = 32;
			constexpr float radius = 0.5f;
			constexpr float step = 1.0f / (float)division;
			TArray<Asset::AssetVertex> vertices;
			TArray<uint32> indices;

			vertices.Resize((division + 1) * (division + 1));
			uint32 index = 0;
			for (uint32 j = 0; j <= division; j++) {
				float v = (float)j * step;
				for (uint32 i = 0; i <= division; i++) {
					float u = (float)i * step;
					float x = radius * Math::Cos(u * 2.0f * Math::PI) * Math::Sin(v * Math::PI);
					float z = radius * Math::Sin(u * 2.0f * Math::PI) * Math::Sin(v * Math::PI);
					float y = radius * Math::Cos(v * Math::PI);
					vertices[index++] = { {x, y, z}, {x, y, z}, {}, { u, v } };
				}
			}
			index = 0;
			indices.Resize(division * division * 6);
			for (uint32 i = 0; i < division; i++) {
				for (uint32 j = 0; j < division; j++) {
					indices[index++] = i * (division + 1) + j;
					indices[index++] = (i + 1) * (division + 1) + j;
					indices[index++] = (i + 1) * (division + 1) + j + 1;
					indices[index++] = i * (division + 1) + j;
					indices[index++] = (i + 1) * (division + 1) + j + 1;
					indices[index++] = i * (division + 1) + j + 1;
				}
			}
			// calc tangent
			const uint32 triangleCount = indices.Size() / 3;
			for (uint32 i = 0; i < triangleCount; ++i) {
				const uint32 i0 = indices[i * 3];
				const uint32 i1 = indices[i * 3 + 1];
				const uint32 i2 = indices[i * 3 + 2];
				FVector3 tangent = CalcTangentVec(vertices[i0].Position, vertices[i1].Position, vertices[i2].Position,
					vertices[i0].UV, vertices[i1].UV, vertices[i2].UV);
				vertices[i0].Tangent = vertices[i1].Tangent = vertices[i1].Tangent = tangent;
			}
			Asset::MeshAsset a;
			a.Name = "Sphere";
			auto& primitive = a.Primitives.EmplaceBack();
			primitive.BinaryFile = "Meshes/Sphere.primitive";
			primitive.Vertices.Swap(vertices);
			primitive.Indices.Swap(indices);
			a.ExportPrimitiveFile(primitive.BinaryFile.c_str(), primitive.Vertices, primitive.Indices);
			Asset::AssetLoader::SaveProjectAsset(&a, "Meshes/Sphere.mesh");

		}
	}
}

namespace Editor {
	
	PathNode::PathNode(const File::FPath& path, NodeID id, NodeID parent):m_ID(id), m_ParentID(parent), m_Path(path) {
		m_PathStr = m_Path.string();
		std::replace(m_PathStr.begin(), m_PathStr.end(), '\\', '/');
		m_Name = m_Path.filename().string();
		m_Ext = m_Path.has_extension() ?  m_Path.extension().string() : "";
	}

	File::FPath PathNode::GetFullPath() const {
		File::FPath path{ Asset::AssetLoader::AssetPath() };
		path.append(m_Path.string());
		return path;
	}


	bool FolderNode::Contains(NodeID id) const {
		return m_ID < id;
	}

	void FileNode::Save() {
		if(Asset::AssetLoader::SaveProjectAsset(m_Asset.Get(), m_PathStr.c_str())) {
			LOG_INFO("FileNode::Save %s", GetPathStr().c_str());
		}
	}

	NodeID AssetManager::InsertFolder(const File::FPath& path, NodeID parent) {
		const NodeID id = static_cast<NodeID>(m_Folders.Size());
		m_Folders.PushBack({ File::RelativePath(path, m_RootPath), id, parent });
		FolderNode* parentNode = GetFolder(parent);
		if(parentNode){
			parentNode->m_Folders.PushBack(id);
		}
		return id;
	}

	NodeID AssetManager::InsertFile(const File::FPath& path, NodeID parent) {
		const NodeID id = static_cast<NodeID>(m_Files.Size());
		m_Files.PushBack({ File::RelativePath(path, m_RootPath), id, parent});
		FolderNode* parentNode = GetFolder(parent);
		if(parentNode){
			parentNode->m_Files.PushBack(id);
		}
		return id;
	}


	void AssetManager::RemoveFile(NodeID id) {
		FileNode* node = GetFile(id);
		if(!node) {
			return;
		}
		FolderNode* parentFolder = GetFolder(node->m_ParentID);
		parentFolder->m_Files.SwapRemove(id);
		parentFolder->m_Files.SwapRemove(id);
		PathNode* swappedAsset = &m_Files.Back();
		m_Files.SwapRemoveAt(id);

		if (swappedAsset->m_ParentID != INVALLID_NODE) {
			FolderNode* backParent = GetFolder(swappedAsset->m_ParentID);
			backParent->m_Files.Replace(swappedAsset->m_ID, id);
			swappedAsset->m_ID = id;
		}
	}

	void AssetManager::RemoveFolder(NodeID id) {
		FolderNode* node = GetFolder(id);
		if(!node) {
			return;
		}
		FolderNode* parentFolder = GetFolder(node->m_ParentID);
		parentFolder->m_Folders.SwapRemove(id);
		PathNode* swappedAsset = &m_Folders.Back();
		m_Folders.SwapRemoveAt(id);

		if(swappedAsset && swappedAsset->m_ParentID != INVALLID_NODE) {
			FolderNode* backParent = GetFolder(swappedAsset->m_ParentID);
			backParent->m_Folders.Replace(swappedAsset->m_ID, id);
			swappedAsset->m_ID = id;
		}
	}

	NodeID AssetManager::BuildFolderRecursively(const File::FPath& path, NodeID parent) {
		using namespace File;
		//the folder node
		NodeID folder = InsertFolder(path, parent);
		FPathIterator iter(path);
		for(const FPathEntry& child: iter) {
			if(child.is_directory()) {
				BuildFolderRecursively(child, folder);
			}
			else {
				InsertFile(child.path(), folder);
			}
		}
		if(m_OnFolderRebuild) {
			m_OnFolderRebuild(GetFolder(folder));
		}
		return folder;
	}

	AssetManager::AssetManager(const char* rootPath) {
		m_RootPath = rootPath;
		BuildTree();
	}

	void AssetManager::BuildTree() {
		m_Folders.Reset();
		m_Files.Reset();
		m_Root = BuildFolderRecursively(m_RootPath, INVALLID_NODE);
		m_Folders[m_Root].m_Name = m_RootPath.parent_path().stem().string();
	}

	FileNode* AssetManager::GetFile(NodeID id) {
		return id < m_Files.Size() ? &m_Files[id] : nullptr;
	}

	FileNode* AssetManager::GetFile(const File::FPath& path) {
		FolderNode* folder = GetRoot();
		for(auto iter: path) {
			if(!folder) {
				break;
			}
			//foreach all files
			if(iter.has_extension()) {
				for (NodeID childID : folder->GetChildFiles()) {
					FileNode* childFile = GetFile(childID);
					if (childFile->GetPath().filename() == iter) {
						return childFile;
					}
				}
			}
			else {
				for (NodeID childID : folder->GetChildFolders()) {
					FolderNode* childFolder = GetFolder(childID);
					auto stem = childFolder->GetPath().stem();
					if (stem == iter) {
						folder = childFolder;
					}
				}
			}
		}
		LOG_INFO("AssetManager::GetFile failed: %s", path.string().c_str());
		return nullptr;
	}

	FolderNode* AssetManager::GetFolder(NodeID id) {
		return id < m_Folders.Size() ? &m_Folders[id] : nullptr;
	}

	FolderNode* AssetManager::GetRoot() {
		return GetFolder(m_Root);
	}

	NodeID AssetManager::RootID() {
		return m_Root;
	}

	void AssetManager::ImportAsset(const char* srcFile, const char* dstFile) {
		if(StrEndsWith(srcFile, ".png")) {
			ImportTexture2DAsset(srcFile, dstFile);
		}
		else if (StrEndsWith(srcFile, ".glb") || StrEndsWith(srcFile, "fbx")) {
			ImportMeshAsset(srcFile, dstFile);
		}
	}
}
