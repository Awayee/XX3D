#pragma once
#include "Core/Public/TArray.h"
#include "Core/Public/Container.h"
#include "Core/Public/TUniquePtr.h"

namespace Object {
	//  A simple ECS framework, reference: https://austinmorlan.com/posts/entity_component_system/

	typedef uint32 EntityID;
	typedef uint8 ComponentID;
	typedef uint32 ComponentMask;


	//  ================ component ====================
	constexpr ComponentID NUM_COMPONENT_MAX = 32;
	ComponentID RegisterComponent();
#define REGISTER_ECS_COMPONENT(cls)\
	public:\
	static Object::ComponentID cls::GetComponentID(){ return s_ComponentID;	}\
	static Object::ComponentMask cls::GetComponentMask(){return 1 << s_ComponentID;}\
	private:\
		inline static Object::ComponentID s_ComponentID{Object::RegisterComponent()}

	class ECSScene;

	class ECSComponentContainerBase {
	public:
		virtual ~ECSComponentContainerBase() = default;
		virtual void Remove(EntityID entityID) = 0;
	};

	// Component container contains same types.
	template<class T>
	class ECSComponentContainer : public ECSComponentContainerBase{
	public:
		ECSComponentContainer() = default;
		~ECSComponentContainer() override = default;
		template<class ...Args>
		T* Add(EntityID entityID, Args&&...args) {
			EntityID idx = m_Components.Size();
			m_Entity2Indices[entityID] = idx;
			m_Components.EmplaceBack(MoveTemp(args)...);
			m_Entities.PushBack(entityID);
			return &m_Components.Back();
		}

		T* Get(EntityID entityID) {
			if(auto iter = m_Entity2Indices.find(entityID); iter != m_Entity2Indices.end()) {
				return &m_Components[iter->second];
			}
			return nullptr;
		}
		void Remove(EntityID entityID) override {
			if (auto iter = m_Entity2Indices.find(entityID); iter != m_Entity2Indices.end()) {
				const EntityID removedIdx = iter->second;
				const EntityID backEntityID = m_Entities.Back();
				m_Components.SwapRemoveAt(removedIdx);
				m_Entities.SwapRemoveAt(removedIdx);
				m_Entity2Indices[backEntityID] = removedIdx;
				m_Entity2Indices.erase(entityID);
			}
		}
	private:
		TArray<T> m_Components;
		TArray<EntityID> m_Entities;
		TUnorderedMap<EntityID, uint32> m_Entity2Indices;
	};

	class ECSComponentContainerMgr {
	public:
		template<class T> T* AddComponent(EntityID entityID) {
			return TryGetComponentContainer<T>()->Add(entityID);
		}
		template<class T> T* GetComponent(EntityID entityID) {
			return TryGetComponentContainer<T>()->Get(entityID);
		}
		template<class T> void RemoveComponent(EntityID entityID) {
			TryGetComponentContainer<T>()->Remove(entityID);
		}
		void RemoveEntity(EntityID entityID) {
			for(auto& container: m_Containers) {
				container->Remove(entityID);
			}
		}
	private:
		TArray<TUniquePtr<ECSComponentContainerBase>> m_Containers;
		template<class T> ECSComponentContainer<T>* TryGetComponentContainer() {
			const ComponentID componentID = T::GetComponentID();
			if(componentID >= m_Containers.Size()) {
				m_Containers.Resize(componentID + 1);
			}
			if(!m_Containers[componentID]) {
				m_Containers[componentID].Reset(new ECSComponentContainer<T>());
			}
			return (ECSComponentContainer<T>*)m_Containers[componentID].Get();
		}
	};

	//  ================ system ====================
	class ECSSystemBase {
	public:
		virtual ~ECSSystemBase() = default;
		void AddEntity(EntityID entityID) {
			m_Entity2Indices[entityID] = m_Entities.Size();
			m_Entities.PushBack(entityID);
		}
		void RemoveEntity(EntityID entityID) {
			if(auto iter = m_Entity2Indices.find(entityID); iter != m_Entity2Indices.end()) {
				const uint32 idx = iter->second;
				const EntityID lastEntity = m_Entities.Back();
				m_Entities.SwapRemoveAt(idx);
				m_Entity2Indices.erase(entityID);
				m_Entity2Indices[lastEntity] = idx;
			}
		}
	protected:
		friend ECSScene;
		TArray<EntityID> m_Entities;
		TMap<EntityID, uint32> m_Entity2Indices;
		virtual void PreUpdate(ECSScene* ecsScene) {/*Do nothing*/ }
		virtual void UpdateEntry(ECSScene* ecsScene, ECSComponentContainerMgr& mgr) = 0;
	};

	template<class ...T>
	class ECSSystem: public ECSSystemBase {
	public:
		static ComponentMask GetComponentMask() { return (0 | ... | T::GetComponentMask()); } // only for c++17
		virtual void Update(ECSScene* ecsScene, T*...components) = 0;
	private:
		void UpdateEntry(ECSScene* ecsScene, ECSComponentContainerMgr& mgr) final {
			for(EntityID entityID: m_Entities) {
				Update(ecsScene, mgr.GetComponent<T>(entityID)...);
			}
		}
	};


	// Scene
	class ECSScene {
	public:
		ECSScene() = default;
		~ECSScene() = default;

		template<class T> void RegisterSystem() {
			m_Systems[T::GetComponentMask()].Reset(new T());
		}

		EntityID NewEntity() {
			EntityID entityID = m_MaxEntity++;
			m_EntityMasks[entityID] = 0;
			return entityID;
		}

		template<class T> T* AddComponent(EntityID entityID) {
			if(auto maskIter = m_EntityMasks.find(entityID); maskIter != m_EntityMasks.end()) {
				maskIter->second |= T::GetComponentMask();
				if(auto sysIter = m_Systems.find(maskIter->second); sysIter != m_Systems.end()) {
					sysIter->second->AddEntity(entityID);
				}
				return m_ComponentContainerMgr.AddComponent<T>(entityID);
			}
			LOG_ERROR("[ECSScene::AddComponent] Invalid entity ID! %u", entityID);
			return nullptr;
		}

		template<class T> T* GetComponent(EntityID entityID) {
			return m_ComponentContainerMgr.GetComponent<T>(entityID);
		}

		template<class T> void RemoveComponent(EntityID entityID) {
			if (auto maskIter = m_EntityMasks.find(entityID); maskIter != m_EntityMasks.end()) {
				if (auto sysIter = m_Systems.find(maskIter->second); sysIter != m_Systems.end()) {
					sysIter->second->RemoveEntity(entityID);
				}
				maskIter->second &= ~T::GetComponentMask();
				m_ComponentContainerMgr.RemoveComponent<T>(entityID);
			}
			else {
				LOG_ERROR("[ECSScene::RemoveComponent] Invalid entity ID! %u", entityID);
			}
		}

		void RemoveEntity(EntityID entityID) {
			if(auto iter = m_EntityMasks.find(entityID); iter != m_EntityMasks.end()) {
				for (auto& [comMask, sys] : m_Systems) {
					if (comMask & iter->second) {
						sys->RemoveEntity(entityID);
					}
				}
				m_EntityMasks.erase(entityID);
				m_ComponentContainerMgr.RemoveEntity(entityID);
			}
		}

		void SystemUpdate() {
			for(auto&[comMask, sys]: m_Systems) {
				sys->PreUpdate(this);
				sys->UpdateEntry(this, m_ComponentContainerMgr);
			}
		}

	private:
		EntityID m_MaxEntity{ 0 };
		ECSComponentContainerMgr m_ComponentContainerMgr;
		TUnorderedMap<EntityID, ComponentMask> m_EntityMasks;
		TUnorderedMap<uint32, TUniquePtr<ECSSystemBase>> m_Systems;
	};
}

// example code
#if false
struct Transform {
	Math::FVector3 Position;
	Math::FVector3 Scale;
	Math::FQuaternion Rotation;
	REGISTER_ECS_COMPONENT(Transform);
};

struct StaticMesh {
	//...
	REGISTER_ECS_COMPONENT(StaticMesh);
};

class MeshRenderSys: public Object::ECSSystem<Transform, StaticMesh> {
public:
	void Update(Object::ECSScene* scene, Transform* transform, StaticMesh* staticMesh) override { /*Do something*/ }
};

void CreatMeshObject(Object::ECSScene* scene) {
	Object::EntityID entityID = scene->NewEntity();
	scene->AddComponent<Transform>(entityID)->Position = { 0,0,0 };
	scene->AddComponent<StaticMesh>(entityID);
}

void RegisterComponents(Object::ECSScene* scene) {
	scene->RegisterSystem<MeshRenderSys>();
}
#endif