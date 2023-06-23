#include "Functions/Public/EditorLevelMgr.h"
#include "Resource/Public/Config.h"
#include "Functions/Public/AssetManager.h"

namespace Editor {

	EditorLevelMgr::EditorLevelMgr() {
		File::FPath startLevel = Engine::GetConfig().StartLevel;
		ALevelAsset* asset;
		if(File::Exist(startLevel)) {
			auto node = ProjectAssetMgr::Instance()->GetFile(startLevel);
			asset = node->GetAsset<ALevelAsset>();
		}
		else {
			m_TempLevel.reset(new ALevelAsset);
			asset = m_TempLevel.get();
		}
		LoadLevel(asset, "");
	}

	EditorLevelMgr::~EditorLevelMgr() {
	}

	ALevelAsset* EditorLevelMgr::CurLevelAsset() {
		return m_LevelAsset;
	}

	void EditorLevelMgr::SaveCurLevel() {
		if(!m_LevelAsset) {
			return;
		}

		if (m_LevelPath.empty()) {

		}
		else{
			AssetLoader::SaveProjectAsset(m_LevelAsset, m_LevelPath.string().c_str());
		}
	}

	void EditorLevelMgr::LoadLevel(ALevelAsset* level, const File::FPath& path) {
		m_LevelAsset = level;
		m_LevelPath = path;
		m_Level.reset(new Engine::Level(*m_LevelAsset, Engine::RenderScene::GetDefaultScene()));
	}
}
