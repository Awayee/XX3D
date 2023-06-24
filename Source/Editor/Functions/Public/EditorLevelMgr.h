#pragma once
#include "Asset/Public/LevelAsset.h"
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/SmartPointer.h"
#include "Functions/Public/EditorLevel.h"

namespace Editor {

	struct LevelObject {
		String Name;
		Engine::RenderObject3D* RenderObj;
	};

	class EditorLevelMgr: public TSingleton<EditorLevelMgr> {
		friend TSingleton<EditorLevelMgr>;
	private:
		TUniquePtr<ALevelAsset> m_TempLevel; //if there is not level asset, create a temp level
		TUniquePtr<EditorLevel> m_Level;
		File::FPath m_LevelPath;
		ALevelAsset* m_LevelAsset {nullptr};
		uint32 m_SelectIndex{ UINT32_MAX };
		EditorLevelMgr();
		~EditorLevelMgr();
	public:
		void LoadLevel(ALevelAsset* asset, const File::FPath& path);
		void SetLevelPath(File::PathStr path);
		void ReloadLevel();
		bool SaveLevel();
		EditorLevel* GetLevel();
		ALevelAsset* GetLevelAsset();
		void SetSelected(uint32 idx);
		uint32 GetSelected();
	};
}