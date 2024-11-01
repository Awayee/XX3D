#include "Objects/Public/Level.h"
#include "Objects/Public/Camera.h"
#include "Asset/Public/AssetLoader.h"
#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/DirectionalLight.h"

namespace Object {

	TArrayView<LevelComponentBase*> LevelComponentContainer::GetComponents() {
		return TArrayView{ (LevelComponentBase**)m_Components.Data(), m_Components.Size() };
	}

	void LevelComponentContainer::RemoveComponent(LevelComponentBase* component) {
		for (uint32 i = 0; i < m_Components.Size(); ++i) {
			if (m_Components[i] == component) {
				component->OnRemove();
				m_Components.SwapRemoveAt(i);
				break;
			}
		}
	}

	ActorComponent::ActorComponent(LevelComponentContainer* actor): m_Actor((LevelActor*)actor) {}

	RenderScene* ActorComponent::GetScene() {
		return m_Actor->GetScene();
	}

	uint32 ActorComponent::GetEntityID() {
		return m_Actor->GetEntityID();
	}

	LevelComponent::LevelComponent(LevelComponentContainer* level) : m_Level((Level*)level) {}

	RenderScene* LevelComponent::GetScene() {
		return m_Level->GetScene();
	}

	LevelActor::LevelActor(Level* level, XString&& name): m_Level(level), m_Name(MoveTemp(name)), m_EntityID(INVALID_ENTITY) {
		if(Object::RenderScene* scene = GetScene()) {
			m_EntityID = scene->NewEntity();
		}
	}

	LevelActor::~LevelActor() {
		if(INVALID_ENTITY != m_EntityID) {
			GetScene()->RemoveEntity(m_EntityID);
		}
	}

	Object::RenderScene* LevelActor::GetScene() const {
		return m_Level->GetScene();
	}

	Level::Level(RenderScene* scene): m_Scene(scene) {}

	bool Level::LoadFile(const char* file) {
		Json::Document doc;
		XString fullPath = Asset::AssetLoader::GetAbsolutePath(file).string();
		if(!Json::ReadFile(fullPath.c_str(), doc, false)) {
			LOG_ERROR("Failed to load level: %s", file);
			return false;
		}

		// level components
		m_Components.Reset();
		if(doc.HasMember("Components")) {
			const Json::Value& componentMetas = doc["Components"];
			m_Components.Reserve(componentMetas.Size());
			for (uint32 i = 0; i < componentMetas.Size(); ++i) {
				const Json::Value& componentMeta = componentMetas[i].GetObject();
				const char* typeName = componentMeta["Name"].GetString();
				auto* typeInfo = LevelComponentFactory::Instance().GetTypeInfo(typeName);
				if (LevelComponentBase* component = typeInfo->AddComponent(this)) {
					component->OnLoad(componentMeta);
				}
			}
		}

		// actors and components
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
					if(LevelComponentBase* component = typeInfo->AddComponent(actor)) {
						component->OnLoad(componentMeta);
					}
				}
			}
		}
		return true;
	}

	void Level::Empty() {
		m_Actors.Reset();
	}

	LevelActor* Level::AddActor(XString&& name) {
		TUniquePtr<LevelActor> actorPtr(new LevelActor(this, MoveTemp(name)));
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

	TypeInfoBase* LevelComponentFactory::GetTypeInfo(LevelComponentBase* component) {
		return GetTypeInfo(component->GetTypeID());
	}
}
