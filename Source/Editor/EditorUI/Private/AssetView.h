#pragma once
#include "Functions/Public/AssetManager.h"
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/TUniquePtr.h"
#include "Asset/Public/TextureAsset.h"

namespace Editor {

	class AssetViewBase {
		friend class WndAssetBrowser;
	protected:
		NodeID m_ID;
	public:
		virtual ~AssetViewBase() {}
		virtual const XString& GetName() = 0;
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
		const XString& GetName() override;
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
		const XString& GetName() override;
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
	class MeshAssetView : public FileAssetViewTemplate<Asset::MeshAsset> {
	public:
		MeshAssetView(FileNode* node) : FileAssetViewTemplate<Asset::MeshAsset>(node) {}
		void Open() override;
	};

	class TextureAssetView: public FileAssetViewTemplate<Asset::TextureAsset> {
	public:
		TextureAssetView(FileNode* node) : FileAssetViewTemplate<Asset::TextureAsset>(node) {}
		void Open() override;
	};

	class LevelAssetView: public FileAssetView {
	public:
		using FileAssetView::FileAssetView;
		void Open() override;
	};

	TUniquePtr<AssetViewBase> CreateAssetView(FileNode* node);
}
