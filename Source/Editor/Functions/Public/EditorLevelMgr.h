#pragma once
#include "Asset/Public/LevelAsset.h"
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/SmartPointer.h"
#include "Objects/Public/Level.h"

namespace Editor {

	class EditorLevelMgr: public TSingleton<EditorLevelMgr> {
		friend TSingleton<EditorLevelMgr>;
	private:
		TUniquePtr<ALevelAsset> m_TempLevel; //if there is not level asset, create a temp level
		TUniquePtr<Engine::Level> m_Level;
		ALevelAsset* m_LevelAsset;
		File::FPath m_LevelPath;
		EditorLevelMgr();
		~EditorLevelMgr();
	public:
		ALevelAsset* CurLevelAsset();
		void SaveCurLevel();
		void LoadLevel(ALevelAsset* level, const File::FPath& path);
	};
}