#include "Functions/Public/AssetManager.h"
#include "Core/Public/File.h"
#include "Core/Public/Log.h"
#include "Core/Public/String.h"
#include "Functions/Public/AssetImporter.h"
#include "Math/Public/Math.h"
#include "System/Public/Configuration.h"


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
				{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {1.0f,  1.0f}},
				{{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {1.0f,  0.0f}},
				{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {1.0f,  1.0f}},
				{{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {0.0f,  0.0f}},
				{{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {},  {0.0f,  1.0f}},

				{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {1.0f,  1.0f}},
				{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {},  {0.0f,  0.0f}},

				{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {1.0f,  1.0f}},
				{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {0.0f,  0.0f}},
				{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {},  {1.0f,  0.0f}},

				{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {1.0f,  1.0f}},
				{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {1.0f,  0.0f}},
				{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {0.0f,  1.0f}},
				{{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {},  {0.0f,  0.0f}},

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
			Asset::PrimitiveAsset p;
			p.Vertices.Swap(vertices);
			p.Indices.Swap(indices);
			Asset::AssetLoader::SaveProjectAsset(&p, primitive.BinaryFile.c_str());
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
			Asset::PrimitiveAsset p;
			p.Vertices.Swap(vertices);
			p.Indices.Swap(indices);
			Asset::AssetLoader::SaveProjectAsset(&p, primitive.BinaryFile.c_str());
			Asset::AssetLoader::SaveProjectAsset(&a, "Meshes/Cube.mesh");

		}
	}
}

namespace Editor {

	void PathNode::ResetPath(File::FPath&& relativePath, NodeID parent) {
		m_RelativePath = MoveTemp(relativePath);
		m_ParentID = parent;
		m_RelativePathStr = m_RelativePath.string();
		std::replace(m_RelativePathStr.begin(), m_RelativePathStr.end(), '\\', '/');
		m_Name = m_RelativePath.filename().string();
		m_Ext = m_RelativePath.has_extension() ? m_RelativePath.extension().string() : "";
	}

	File::FPath PathNode::GetFullPath() const {
		return Asset::AssetLoader::GetAbsolutePath(m_RelativePathStr.c_str());
	}


	bool FolderNode::Contains(const FolderNode* folderNode) const {
		if(m_ID == folderNode->m_ID) {
			return false;
		}
		return File::IsSubPathOf(folderNode->m_RelativePath, m_RelativePath);
	}

	AssetManager::AssetManager(File::FPath&& rootPath) {
		m_RootPath = MoveTemp(rootPath);
		BuildTree();
	}

	FileNode* AssetManager::GetFileNode(NodeID id) {
		return (INVALID_NODE != id &&  id < m_Files.Size()) ? &m_Files[id] : nullptr;
	}

	FolderNode* AssetManager::GetFolderNode(NodeID id) {
		return( INVALID_NODE != id && id < m_Folders.Size()) ? &m_Folders[id] : nullptr;
	}

	FileNode* AssetManager::GetFileNode(const File::FPath& path) {
		FolderNode* folder = GetRootNode();
		for (auto iter : path) {
			const XString iterString = iter.string();
			// node is file, find the file to return
			if (iter.has_extension()) {
				for (NodeID childID : folder->GetChildFiles()) {
					FileNode* childFile = GetFileNode(childID);
					if (childFile->GetName() == iterString) {
						return childFile;
					}
				}
				break;
			}
			// node is folder, find a folder for next iteration
			bool found = false;
			for (NodeID childID : folder->GetChildFolders()) {
				FolderNode* childFolder = GetFolderNode(childID);
				if (childFolder->GetName() == iterString) {
					folder = childFolder;
					found = true;
				}
			}
			if (!found) {
				break;
			}
		}
		LOG_WARNING("AssetManager::GetFileNode failed: %s", path.string().c_str());
		return nullptr;
	}

	FolderNode* AssetManager::GetFolderNode(const File::FPath& path) {
		FolderNode* folder = GetRootNode();
		for (auto iter : path) {
			const XString iterString = iter.string();
			bool found = false;
			for (NodeID childID : folder->GetChildFolders()) {
				FolderNode* childFolder = GetFolderNode(childID);
				if (childFolder->GetName() == iterString) {
					folder = childFolder;
					found = true;
				}
			}
			if (!found) {
				LOG_WARNING("AssetManager::GetFolderNode failed: %s", path.string().c_str());
				return nullptr;
			}
		}
		return folder;
	}

	FolderNode* AssetManager::GetRootNode() {
		return GetFolderNode(m_Root);
	}

	NodeID AssetManager::CreateFolder(File::FPath&& relativePath, NodeID parent) {
		const NodeID id = m_Folders.Size();
		m_Folders.EmplaceBack(this, id).ResetPath(MoveTemp(relativePath), parent);
		FolderNode* parentNode = GetFolderNode(parent);
		if (parentNode) {
			parentNode->m_Folders.PushBack(id);
			if(m_OnFolderModified) {
				m_OnFolderModified(parentNode);
			}
		}
		return id;
	}

	NodeID AssetManager::CreateFile(File::FPath&& relativePath, NodeID parent) {
		if (FolderNode* parentNode = GetFolderNode(parent)) {
			for(const NodeID id: parentNode->m_Files) {
				FileNode* fileNode = GetFileNode(id);
				if(fileNode->GetPath() == relativePath) {
					fileNode->ResetPath(MoveTemp(relativePath), parent);
					return id;
				}
			}
			const NodeID id = m_Files.Size();
			m_Files.EmplaceBack(this, id).ResetPath(MoveTemp(relativePath), parent);
			parentNode->m_Files.PushBack(id);
			if (m_OnFolderModified) {
				m_OnFolderModified(parentNode);
			}
			return id;
		}
		return INVALID_NODE;
	}

	void AssetManager::ReloadFolder(NodeID nodeId, bool recursively) {
		FolderNode* node = GetFolderNode(nodeId);
		if(!node) {
			return;
		}
		auto& folders = node->m_Folders;
		auto& files = node->m_Files;
		uint32 folderSize = 0, fileSize = 0;
		File::DirIterator iter(node->GetFullPath());
		for (const File::DirEntry& entry : iter) {
			File::FPath relativePath = File::RelativePath(entry.path(), m_RootPath);
			if (entry.is_directory()) {
				if (folderSize < folders.Size()) {
					GetFolderNode(folders[folderSize])->ResetPath(MoveTemp(relativePath), nodeId);
				}
				else {
					folders.PushBack(CreateFolder(MoveTemp(relativePath), nodeId));
				}
				if (recursively) {
					ReloadFolder(folders[folderSize], true);
				}
				++folderSize;
			}
			else {
				if (fileSize < files.Size()) {
					GetFileNode(files[fileSize])->ResetPath(File::RelativePath(entry.path(), m_RootPath), nodeId);
				}
				else {
					files.PushBack(CreateFile(MoveTemp(relativePath), nodeId));
				}
				++fileSize;
			}
		}
		// remove rest items
		for(uint32 i=folders.Size(); i>folderSize; --i) {
			m_Folders[files.Back()].m_ParentID = INVALID_NODE;
			RemoveFolder(folders.Back());
			folders.PopBack();
		}
		for(uint32 i=files.Size(); i>fileSize; --i) {
			m_Files[files.Back()].m_ParentID = INVALID_NODE;
			RemoveFile(files.Back());
			files.PopBack();
		}
		// trigger event
		if(m_OnFolderModified) {
			m_OnFolderModified(node);
		}
	}

	NodeID AssetManager::RootID() {
		return m_Root;
	}

	void AssetManager::RemoveFile(NodeID id) {
		FileNode* node = GetFileNode(id);
		if(!node) {
			return;
		}
		if(FolderNode* parentFolder = GetFolderNode(node->m_ParentID)) {
			parentFolder->m_Files.SwapRemove(id);
		}
		PathNode& swappedAsset = m_Files.Back();
		m_Files.SwapRemoveAt(id);

		if (swappedAsset.m_ParentID != INVALID_NODE) {
			if(FolderNode* backParent = GetFolderNode(swappedAsset.m_ParentID)) {
				backParent->m_Files.Replace(swappedAsset.m_ID, id);
				swappedAsset.m_ID = id;
			}
		}
	}

	void AssetManager::RemoveFolder(NodeID id) {
		FolderNode* node = GetFolderNode(id);
		if(!node) {
			return;
		}
		if(FolderNode* parentFolder = GetFolderNode(node->m_ParentID)) {
			parentFolder->m_Folders.SwapRemove(id);
		}
		PathNode* swappedAsset = &m_Folders.Back();
		m_Folders.SwapRemoveAt(id);
		if(swappedAsset && swappedAsset->m_ParentID != INVALID_NODE) {
			FolderNode* backParent = GetFolderNode(swappedAsset->m_ParentID);
			backParent->m_Folders.Replace(swappedAsset->m_ID, id);
			swappedAsset->m_ID = id;
		}
	}

	NodeID AssetManager::BuildFolderRecursively(const File::FPath& path, NodeID parent) {
		//the folder node
		NodeID folder = CreateFolder(File::RelativePath(path, m_RootPath), parent);
		File::DirIterator iter(path);
		for(const File::DirEntry& child: iter) {
			if(child.is_directory()) {
				BuildFolderRecursively(child, folder);
			}
			else {
				CreateFile(File::RelativePath(child.path(), m_RootPath), folder);
			}
		}
		return folder;
	}

	void AssetManager::BuildTree() {
		m_Folders.Reset();
		m_Files.Reset();
		m_Root = BuildFolderRecursively(m_RootPath, INVALID_NODE);
		m_Folders[m_Root].m_Name = m_RootPath.parent_path().stem().string();
	}

	EngineAssetMgr::EngineAssetMgr() : AssetManager(Engine::EngineConfig::Instance().GetEngineAssetDir()) {}

	ProjectAssetMgr::ProjectAssetMgr() : AssetManager(Engine::EngineConfig::Instance().GetProjectAssetDir()) {}
}
