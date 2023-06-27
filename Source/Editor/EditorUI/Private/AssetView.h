#pragma once
#include "Functions/Public/AssetManager.h"
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/SmartPointer.h"
#include "Asset/Public/TextureAsset.h"
#include "Asset/Public/LevelAsset.h"

namespace Editor {

	class AssetViewBase {
		friend class WndAssetBrowser;
	protected:
		NodeID m_ID;
		String m_Name;
		String m_Icon;
	public:
		virtual ~AssetViewBase() {}

		virtual bool IsFolder() = 0;
		virtual void Open() = 0;
		virtual void Save() = 0;
		virtual void OnDrag() = 0;
	};

	class FolderAssetView final : public AssetViewBase {
	private:
		FolderNode* m_Node{ nullptr };
	public:
		FolderAssetView(FolderNode* node);
		bool IsFolder() override;
		void Open() override;
		void Save() override;
		void OnDrag() override;
	};

	class FileAssetView : public AssetViewBase {
	protected:
		FileNode* m_Node = nullptr;
	public:
		FileAssetView(FileNode* node);
		bool IsFolder() override;
		virtual void Open() override {}
		virtual void Save() override {}
		virtual void OnDrag() override;
	};

	template<typename T>
	class FileAssetViewTemplate : public FileAssetView {
	protected:
		T* m_Asset = nullptr;
	public:
		FileAssetViewTemplate(FileNode* node): FileAssetView(node){
			m_Asset = m_Node->GetAsset<T>();
		}
	};

	// asset extents
	class MeshAssetView : public FileAssetViewTemplate<AMeshAsset> {
	public:
		MeshAssetView(FileNode* node) : FileAssetViewTemplate<AMeshAsset>(node) {}
		void Open() override;
	};

	class TextureAssetView: public FileAssetViewTemplate<ATextureAsset> {
	public:
		TextureAssetView(FileNode* node) : FileAssetViewTemplate<ATextureAsset>(node) {}
		void Open() override;
	};

	class LevelAssetView: public FileAssetViewTemplate<ALevelAsset> {
	public:
		LevelAssetView(FileNode* node) : FileAssetViewTemplate<ALevelAsset>(node) {}
		void Open() override;
		void Save() override;
	};

	inline TUniquePtr<AssetViewBase> CreateAssetView(FileNode* node) {
		const String& ext = node->GetPath().extension().string();
		if(ext == ".mesh") {
			return MakeUniquePtr<MeshAssetView>(node);
		}
		if(ext == ".texture") {
			return MakeUniquePtr<TextureAssetView>(node);
		}
		if(ext == ".level") {
			return MakeUniquePtr<LevelAssetView>(node);
		}
		return MakeUniquePtr<FileAssetView>(node);
	}
}
