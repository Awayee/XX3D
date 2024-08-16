#pragma once
#include "Asset/Public/LevelAsset.h"
#include "Asset/Public/MeshAsset.h"
#include "Functions/Public/EditorLevel.h"

namespace Editor {

	struct LevelObject {
		XString Name;
		Object::EntityID ObjectID;
	};

	class EditorLevelMgr {
		SINGLETON_INSTANCE(EditorLevelMgr);
	private:
		TUniquePtr<Asset::LevelAsset> m_TempLevel; //if there is not level asset, create a temp level
		TUniquePtr<EditorLevel> m_Level;
		File::FPath m_LevelPath;
		Asset::LevelAsset* m_LevelAsset {nullptr};
		uint32 m_SelectIndex{ UINT32_MAX };
		EditorLevelMgr();
		~EditorLevelMgr();
	public:
		void LoadLevel(Asset::LevelAsset* asset, const File::FPath& path);
		void SetLevelPath(File::PathStr path);
		void ReloadLevel();
		bool SaveLevel();
		EditorLevel* GetLevel();
		Asset::LevelAsset* GetLevelAsset();
		void SetSelected(uint32 idx);
		uint32 GetSelected();
	};
}