#include "Functions/Public/EditorLevel.h"
#include "Functions/Public/AssetManager.h"
#include "Objects/Public/SkyBox.h"
#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/LevelComponents.h"
#include "System/Public/Configuration.h"

namespace Editor {

	void CameraSave(Object::CameraComponent* com, Json::ValueWriter& val) {
		val.AddFloatArray("Eye", com->Eye.Data(), 3);
		val.AddFloatArray("At", com->At.Data(), 3);
		val.AddFloatArray("Up", com->Up.Data(), 3);
		val.AddMember("ProjType", (int)com->ProjType);
		val.AddMember("Near", com->Near);
		val.AddMember("Far", com->Far);
		val.AddMember("Fov", com->Fov);
		val.AddMember("HalfHeight", com->HalfHeight);
	}
	REGISTER_LEVEL_EDIT_OnSave(Object::CameraComponent, CameraSave);

	void DirectionalLightSave(Object::DirectionalLightComponent* com, Json::ValueWriter& val) {
		val.AddFloatArray("Rotation", com->Rotation.Data(), 3);
		val.AddFloatArray("Color", com->Color.Data(), 4);
		val.AddMember("EnableShadow", com->EnableShadow);
		val.AddMember("ShadowDistance", com->ShadowDistance);
		val.AddMember("ShadowLogDistribution", com->ShadowLogDistribution);
		val.AddMember("ShadowMapSize", com->ShadowMapSize);
		val.AddMember("ShadowBiasConst", com->ShadowBiasConst);
		val.AddMember("ShadowBiasSlope", com->ShadowBiasSlope);
	}
	REGISTER_LEVEL_EDIT_OnSave(Object::DirectionalLightComponent, DirectionalLightSave);

	void TransformSave(Object::TransformComponent* com, Json::ValueWriter& val) {
		val.AddFloatArray("Position", com->Position.Data(), 3);
		val.AddFloatArray("Scale", com->Scale.Data(), 3);
		val.AddFloatArray("Rotation", com->Euler.Data(), 3);
	}
	REGISTER_LEVEL_EDIT_OnSave(Object::TransformComponent, TransformSave);

	void MeshSave(Object::MeshComponent* component, Json::ValueWriter& val) {
		const XString& meshFile = component->GetMeshFile();
		val.AddString("MeshFile", meshFile);
		val.AddMember("CastShadow", component->GetCastShadow());
	}
	REGISTER_LEVEL_EDIT_OnSave(Object::MeshComponent, MeshSave);

	void InstancedDataSave(Object::InstanceDataComponent* component, Json::ValueWriter& val) {
		const XString& instFile = component->GetInstanceFile();
		val.AddString("InstanceFile", instFile);
	}
	REGISTER_LEVEL_EDIT_OnSave(Object::InstanceDataComponent, InstancedDataSave);

	void SkyBoxSave(Object::SkyBoxComponent* component, Json::ValueWriter& val) {
		const XString& cubeMapFile = component->GetCubeMapFile();
		val.AddString("CubeMapFile", cubeMapFile);
	}
	REGISTER_LEVEL_EDIT_OnSave(Object::SkyBoxComponent, SkyBoxSave);

	bool EditorLevel::LoadFile(const char* file) {
		if(!Level::LoadFile(file)) {
			return false;
		}
		return true;
	}

	bool EditorLevel::SaveFile(const char* file) {
		Json::Document doc(Json::Type::kObjectType);
		// scene components
		Json::ValueWriter levelComponentValues(Json::Type::kArrayType, doc);
		for(auto* component: GetComponents()) {
			Json::ValueWriter levelComponentValue(Json::Type::kObjectType, doc);
			levelComponentValue.AddString("Name", Object::LevelComponentFactory::Instance().GetTypeInfo(component)->GetTypeName());
			LevelComponentEditProxyFactory::Instance().GetProxy(component).OnSave(component, levelComponentValue);
			levelComponentValues.PushBack(levelComponentValue);
		}
		levelComponentValues.Write("Components");
		// Actors
		Json::ValueWriter actorValues(Json::Type::kArrayType, doc);
		for(auto& actor: m_Actors) {
			Json::ValueWriter actorValue(Json::Type::kObjectType, doc);
			actorValue.AddString("Name", actor->GetName());
			Json::ValueWriter componentValues(Json::Type::kArrayType, doc);
			for(auto* component: actor->GetComponents()) {
				Json::ValueWriter componentValue(Json::Type::kObjectType, doc);
				componentValue.AddString("Name", Object::LevelComponentFactory::Instance().GetTypeInfo(component)->GetTypeName());
				LevelComponentEditProxyFactory::Instance().GetProxy(component).OnSave(component, componentValue);
				componentValues.PushBack(componentValue);
			}
			actorValue.AddMember("Components", componentValues);
			actorValues.PushBack(actorValue);
		}
		actorValues.Write("Actors");
		XString fullPath = Asset::AssetLoader::GetAbsolutePath(file).string();
		return Json::WriteFile(fullPath.c_str(), doc, false, true);
	}

	uint32 EditorLevel::GetActorSize() {
		return m_Actors.Size();
	}

	Object::LevelActor* EditorLevel::GetActor(uint32 actorID) {
		return m_Actors[actorID];
	}

	void EditorLevel::InitDefault(EditorLevel& level) {
		level.m_Actors.Reset();
		level.m_Components.Reset();
		auto* cameraCom = level.AddComponent<Object::CameraComponent>();
		auto* directionalLightCom = level.AddComponent<Object::DirectionalLightComponent>();
	}

	void EditorLevelMgr::LoadLevel(File::PathStr levelFile) {
		m_Level->LoadFile(levelFile);
		m_LevelFile = levelFile;
	}

	void EditorLevelMgr::SetLevelPath(File::PathStr levelFile) {
		m_LevelFile = levelFile;
	}

	void EditorLevelMgr::ReloadLevel() {
		File::FPath levelPath{ m_LevelFile };
		if (FileNode* node = ProjectAssetMgr::Instance()->GetFileNode(levelPath)) {
			m_Level->LoadFile(m_LevelFile.c_str());
		}
		else {
			m_LevelFile.clear();
			m_Level->Empty();
		}
		m_SelectIndex = INVALID_INDEX;
	}

	bool EditorLevelMgr::SaveLevel() {
		if (m_LevelFile.empty()) {
			return false;
		}
		if(!m_Level->SaveFile(m_LevelFile.c_str())) {
			return false;
		}
		LOG_INFO("[EditorLevelMgr::SaveLevel] Level saved: %s", m_LevelFile.c_str());
		return true;
	}

	EditorLevelMgr::EditorLevelMgr() {
		// initialize type info
		LevelComponentEditProxyFactory::Instance().Initialize();
		m_Level.Reset(new EditorLevel(Object::RenderScene::GetDefaultScene()));
		File::FPath path = Engine::ProjectConfig::Instance().StartLevel;
		if (auto node = ProjectAssetMgr::Instance()->GetFileNode(path)) {
			LoadLevel(node->GetPathStr().c_str());
		}
		else {
			LOG_ERROR("Could not load start level: \"%s\"", path.string().c_str());
		}
	}

	void LevelComponentEditProxyFactory::Initialize() {
		for (auto& initializer : m_InitializerArray) {
			initializer(this);
		}
		m_InitializerArray.Reset();
	}

	const LevelComponentEditProxyFactory::LevelComponentEditProxy& LevelComponentEditProxyFactory::GetProxy(Object::LevelComponentBase* c) {
		const uint32 typeID = c->GetTypeID();
		CHECK(typeID < m_EditProxies.Size());
		return m_EditProxies[typeID];
	}

	LevelComponentEditProxyFactory& LevelComponentEditProxyFactory::Instance() {
		static LevelComponentEditProxyFactory s_Instance{};
		return s_Instance;
	}

	LevelComponentEditProxyFactory::LevelComponentEditProxy& LevelComponentEditProxyFactory::GetProxy(uint32 typeID) {
		if (typeID >= m_EditProxies.Size()) {
			m_EditProxies.Resize(typeID + 1);
		}
		return m_EditProxies[typeID];
	}
}
