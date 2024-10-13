#include "Functions/Public/EditorLevel.h"
#include "Objects/Public/Camera.h"
#include "Functions/Public/AssetManager.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/SkyBox.h"
#include "Objects/Public/StaticMesh.h"
#include "System/Public/Configuration.h"

namespace Editor {
	void TransformSave(Object::TransformComponent* com, Json::ValueWriter& val) {
		auto& transform = com->Transform;
		val.AddFloatArray("Position", transform.Position.Data(), 3);
		val.AddFloatArray("Scale", transform.Scale.Data(), 3);
		Math::FVector3 euler = transform.Rotation.ToEuler();
		val.AddFloatArray("Rotation", euler.Data(), 3);
	}
	REGISTER_LEVEL_EDIT_OnSave(Object::TransformComponent, TransformSave);

	void MeshSave(Object::MeshComponent* component, Json::ValueWriter& val) {
		const XString& meshFile = component->GetMeshFile();
		val.AddString("MeshFile", meshFile);
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
		// camera
		Object::RenderCamera* camera = m_Scene->GetMainCamera();
		auto& view = camera->GetView();
		auto& projection = camera->GetProjectionData();
		Json::ValueWriter cameraVal(Json::Type::kObjectType, doc);
		cameraVal.AddFloatArray("Eye", view.Eye.Data(), 3);
		cameraVal.AddFloatArray("At", view.At.Data(), 3);
		cameraVal.AddFloatArray("Up", view.Up.Data(), 3);
		cameraVal.AddMember("ProjType", (int)projection.ProjType);
		cameraVal.AddMember("Near", projection.Near);
		cameraVal.AddMember("Far", projection.Far);
		cameraVal.AddMember("Fov", projection.Fov);
		cameraVal.AddMember("HalfHeight", projection.HalfHeight);
		cameraVal.Write("Camera");
		// directional 
		Object::DirectionalLight* dLight = m_Scene->GetDirectionalLight();
		Json::ValueWriter dLightVal(Json::Type::kObjectType, doc);
		dLightVal.AddFloatArray("Rotation", dLight->GetRotation().Data(), 3);
		dLightVal.AddFloatArray("Color", dLight->GetColor().Data(), 3);
		dLightVal.Write("DirectionalLight");
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
	}

	void LevelComponentEditProxyFactory::Initialize() {
		for (auto& initializer : m_InitializerArray) {
			initializer(this);
		}
		m_InitializerArray.Reset();
	}

	const LevelComponentEditProxyFactory::LevelComponentEditProxy& LevelComponentEditProxyFactory::GetProxy(Object::LevelComponent* c) {
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
