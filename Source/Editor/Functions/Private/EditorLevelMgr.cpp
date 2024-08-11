#include "Functions/Public/EditorLevelMgr.h"
#include "System/Public/EngineConfig.h"
#include "Functions/Public/AssetManager.h"

namespace Editor {

	EditorLevelMgr::EditorLevelMgr() {
		File::FPath path = Engine::ConfigManager::GetData().StartLevel;
		Asset::LevelAsset* asset;
		auto node = ProjectAssetMgr::Instance()->GetFile(path);
		if(node) {
			asset = node->GetAsset<Asset::LevelAsset>();
			LOG_INFO("Loaded start level: %s", path.string().c_str());
			LoadLevel(asset, path);
		}
		else {
			m_TempLevel.Reset(new Asset::LevelAsset);
			asset = m_TempLevel.Get();
			LoadLevel(asset, "");
		}
	}

	EditorLevelMgr::~EditorLevelMgr() {
	}

	void EditorLevelMgr::LoadLevel(Asset::LevelAsset* asset, const File::FPath& path) {
		m_LevelAsset = asset;
		m_LevelPath = path;
		m_Level.Reset(new EditorLevel(*m_LevelAsset, Object::RenderScene::GetDefaultScene()));
	}

	void EditorLevelMgr::SetLevelPath(File::PathStr path) {
		m_LevelPath = path;
	}

	void EditorLevelMgr::ReloadLevel() {
		FileNode* node = ProjectAssetMgr::Instance()->GetFile(m_LevelPath);
		if(node) {
			m_LevelAsset = node->GetAsset<Asset::LevelAsset>();
			m_Level.Reset(new EditorLevel(*m_LevelAsset, Object::RenderScene::GetDefaultScene()));
		}
		else {
			m_Level.Reset();
		}
	}

	bool EditorLevelMgr::SaveLevel() {
		if(m_LevelPath.empty()) {
			return false;
		}
		m_Level->SaveAsset(m_LevelAsset);
		Asset::AssetLoader::SaveProjectAsset(m_LevelAsset, m_LevelPath.string().c_str());
		LOG_INFO("[EditorLevelMgr::SaveLevel] Level saved: %s", m_LevelPath.string().c_str());
		return true;
	}

	EditorLevel* EditorLevelMgr::GetLevel() {
		return m_Level.Get();
	}

	Asset::LevelAsset* EditorLevelMgr::GetLevelAsset() {
		return m_LevelAsset;
	}

	void EditorLevelMgr::SetSelected(uint32 idx) {
		m_SelectIndex = idx;
	}

	uint32 EditorLevelMgr::GetSelected() {
		return m_SelectIndex;
	}
}
