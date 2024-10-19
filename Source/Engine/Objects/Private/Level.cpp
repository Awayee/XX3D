#include "Objects/Public/Level.h"
#include "Objects/Public/Camera.h"
#include "Asset/Public/AssetLoader.h"
#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/DirectionalLight.h"

namespace Object {
	RenderScene* LevelComponent::GetScene() {
		return m_Owner->GetScene();
	}

	Object::EntityID LevelComponent::GetEntityID() {
		return m_Owner->GetEntityID();
	}

	LevelActor::LevelActor(RenderScene* scene, XString&& name): m_Scene(scene), m_Name(MoveTemp(name)) {
		m_EntityID = scene->NewEntity();
	}

	LevelActor::~LevelActor() {
		m_Scene->RemoveEntity(m_EntityID);
	}

	void LevelActor::RemoveComponent(LevelComponent* component){
		for(uint32 i=0; i<m_Components.Size(); ++i) {
			if(m_Components[i] == component) {
				component->OnRemove();
				m_Components.SwapRemoveAt(i);
				break;
			}
		}
	}

	TArrayView<LevelComponent*> LevelActor::GetComponents() {
		return TArrayView{ (LevelComponent**)m_Components.Data(), m_Components.Size() };
	}

	bool Level::LoadFile(const char* file) {
		Json::Document doc;
		XString fullPath = Asset::AssetLoader::GetAbsolutePath(file).string();
		if(!Json::ReadFile(fullPath.c_str(), doc, false)) {
			LOG_WARNING("Failed to load level: %s", file);
			return false;
		}
		if(doc.HasMember("Camera")) {
			Math::FVector3 eye, at, up;
			const rapidjson::Value& cameraParam = doc["Camera"].GetObject();
			Json::LoadFloatArray(cameraParam["Eye"], eye.Data(), 3);
			Json::LoadFloatArray(cameraParam["At"], at.Data(), 3);
			Json::LoadFloatArray(cameraParam["Up"], up.Data(), 3);
			const Object::EProjType projType = (Object::EProjType)cameraParam["ProjType"].GetInt();
			const float near = cameraParam["Near"].GetFloat();
			const float far = cameraParam["Far"].GetFloat();
			const float fov = cameraParam["Fov"].GetFloat();
			const float halfHeight = cameraParam["HalfHeight"].GetFloat();
			Object::RenderCamera* camera = m_Scene->GetMainCamera();
			const Object::CameraView view{ eye, at, up };
			camera->SetView(view);
			const Object::RenderCamera::ProjectionData projectionData{ projType, near, far, fov, halfHeight };
			camera->SetProjectionData(projectionData);
		}
		if (doc.HasMember("DirectionalLight")) {
			const rapidjson::Value& directionalLightParam = doc["DirectionalLight"].GetObject();
			Math::FVector3 rotation, color;
			Json::LoadFloatArray(directionalLightParam["Rotation"], rotation.Data(), 3);
			Json::LoadFloatArray(directionalLightParam["Color"], color.Data(), 3);
			Object::DirectionalLight* directionalLight = m_Scene->GetDirectionalLight();
			directionalLight->SetRotation(rotation);
			directionalLight->SetColor(color);
		}

		m_Actors.Reset();
		if(doc.HasMember("Actors")) {
			const Json::Value& actorMetas = doc["Actors"];
			m_Actors.Reserve(actorMetas.Size());
			for(uint32 i=0; i<actorMetas.Size(); ++i) {
				const Json::Value& actorMeta = actorMetas[i].GetObject();
				XString actorName = actorMeta["Name"].GetString();
				LevelActor* actor = AddActor(MoveTemp(actorName));
				const Json::Value& componentMetas = actorMeta["Components"].GetArray();
				for(uint32 j=0; j<componentMetas.Size(); ++j) {
					const Json::Value& componentMeta = componentMetas[j].GetObject();
					const char* typeName = componentMeta["Name"].GetString();
					auto* typeInfo = LevelComponentFactory::Instance().GetTypeInfo(typeName);
					LevelComponent* component = typeInfo->AddComponent(actor);
					component->OnLoad(componentMeta);
				}
			}
		}
		return true;
	}

	void Level::Empty() {
	}

	LevelActor* Level::AddActor(XString&& name) {
		TUniquePtr<LevelActor> actorPtr(new LevelActor(m_Scene, MoveTemp(name)));
		actorPtr->m_ArrayIndex = m_Actors.Size();
		m_Actors.PushBack(MoveTemp(actorPtr));
		return m_Actors.Back().Get();
	}

	void Level::RemoveActor(LevelActor* actor) {
		const uint32 arrayIndex = actor->m_ArrayIndex;
		if(arrayIndex < m_Actors.Size()) {
			if(arrayIndex == m_Actors.Size() - 1) {
				m_Actors.PopBack();
			}
			else {
				m_Actors.SwapRemoveAt(arrayIndex);
				m_Actors[arrayIndex]->m_ArrayIndex = arrayIndex;
			}
		}
	}

	LevelComponentFactory& LevelComponentFactory::Instance() {
		static LevelComponentFactory s_Instance{};
		return s_Instance;
	}

	uint32 LevelComponentFactory::RegisterTypeInfo(TUniquePtr<TypeInfoBase>&& typeInfoPtr) {
		const uint32 typeID = m_TypeInfos.Size();
		m_MapTypeNameID[typeInfoPtr->GetTypeName()] = typeID;
		m_TypeInfos.PushBack(MoveTemp(typeInfoPtr));
		return typeID;
	}

	uint32 LevelComponentFactory::GetTypeSize() {
		return m_TypeInfos.Size();
	}

	TypeInfoBase* LevelComponentFactory::GetTypeInfo(uint32 typeID) {
		return m_TypeInfos[typeID];
	}

	TypeInfoBase* LevelComponentFactory::GetTypeInfo(const char* name) {
		if(auto iter=m_MapTypeNameID.find(name); iter!=m_MapTypeNameID.end()) {
			return GetTypeInfo(iter->second);
		}
		return nullptr;
	}

	TypeInfoBase* LevelComponentFactory::GetTypeInfo(LevelComponent* component) {
		return GetTypeInfo(component->GetTypeID());
	}
}
