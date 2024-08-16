#include "ClientCode/Public/Client.h"
#include "Objects/Public/RenderScene.h"
#include "Asset/Public/AssetLoader.h"
#include "Asset/Public/LevelAsset.h"

namespace Runtime {

	Client::Client() {
		Asset::LevelAsset asset;
		Asset::AssetLoader::LoadProjectAsset(&asset, "Level/test.level");
		m_Level.Reset(new Object::Level(asset, Object::RenderScene::GetDefaultScene()));
	}

	void Client::Tick() {
	}

	Client::~Client() {
		m_Level.Reset();
	}
}
