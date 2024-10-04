#pragma once
#include "Objects/Public/RenderScene.h"
#include "Core/Public/Json.h"
#include "Asset/Public/LevelAsset.h"

namespace Object {
	class LevelActor;
	class Level;

	class LevelComponent {
	public:
		LevelComponent(LevelActor* owner): m_Owner(owner){}
		virtual ~LevelComponent() = default;
		virtual void OnLoad(const Json::Value& val) = 0;
		virtual void OnAdd() {/*Do nothing*/ }
		virtual void OnRemove() {/*Do nothing*/ }
		virtual uint32 GetTypeID() = 0;
		template<class T> bool Is() { return T::TypeID == GetTypeID(); }
		LevelActor* GetOwner() { return m_Owner; }
		RenderScene* GetScene();
		Object::EntityID GetEntityID();
	private:
		LevelActor* m_Owner;
	};

	class LevelActor {
	public:
		LevelActor(RenderScene* scene, XString&& name);
		~LevelActor();
		template<class T> T* GetComponent() {
			for (auto& com : m_Components) {
				if (com->Is<T>()) {
					return (T*)com.Get();
				}
			}
			return nullptr;
		}
		template<class T> T* AddComponent() {
			if(T* com = GetComponent<T>()) {
				return com;
			}
			auto& ptr = m_Components.EmplaceBack();
			ptr.Reset(new T(this));
			ptr->OnAdd();
			return (T*)ptr.Get();
		}
		void RemoveComponent(LevelComponent* component);
		const XString& GetName() const { return m_Name; }
		void SetName(const XString& name) { m_Name = name; }
		Object::EntityID GetEntityID() const { return m_EntityID; }
		Object::RenderScene* GetScene() const { return m_Scene; }
		TArrayView<LevelComponent*> GetComponents();
	private:
		friend class Level;
		static constexpr uint32 INVALID_INDEX{ UINT32_MAX };
		Object::RenderScene* m_Scene;
		XString m_Name;
		Object::EntityID m_EntityID;
		TArray<TUniquePtr<LevelComponent>> m_Components;
		uint32 m_ArrayIndex{ INVALID_INDEX };
	};

	class Level {
	public:
		Level(RenderScene* scene) : m_Scene(scene) {}
		~Level() = default;
		RenderScene* GetScene() const { return m_Scene; }
		bool LoadFile(const char* file);
		void Empty();
		LevelActor* AddActor(XString&& name);
		void RemoveActor(LevelActor* actor);
	protected:
		RenderScene* m_Scene;
		TArray<TUniquePtr<LevelActor>> m_Actors;
	};

	// ========= type system =========
	class TypeInfoBase {
	public:
		TypeInfoBase(const char* typeName): m_TypeName(typeName) {}
		virtual ~TypeInfoBase() = default;
		virtual LevelComponent* AddComponent(LevelActor* actor) = 0;
		const XString& GetTypeName() { return m_TypeName; }
	private:
		XString m_TypeName;
	};

	template<class T> class TypeInfo: public TypeInfoBase {
	public:
		using TypeInfoBase::TypeInfoBase;
		LevelComponent* AddComponent(LevelActor* actor) override {
			return actor->AddComponent<T>();
		}
	};

	class LevelComponentFactory {
	public:
		static LevelComponentFactory& Instance();
		uint32 RegisterTypeInfo(TUniquePtr<TypeInfoBase>&& typeInfoPtr);
		uint32 GetTypeSize();
		TypeInfoBase* GetTypeInfo(uint32 typeID);
		TypeInfoBase* GetTypeInfo(const char* name);
		TypeInfoBase* GetTypeInfo(LevelComponent* component);
	private:
		TArray<TUniquePtr<TypeInfoBase>> m_TypeInfos;
		TMap<XString, uint32> m_MapTypeNameID;
		NON_COPYABLE(LevelComponentFactory);
		NON_MOVEABLE(LevelComponentFactory);
		LevelComponentFactory() = default;
		~LevelComponentFactory() = default;
	};
#define REGISTER_LEVEL_COMPONENT(cls) public:\
	using LevelComponent::LevelComponent;\
	uint32 GetTypeID() override { return TypeID; }\
	inline static uint32 TypeID{ LevelComponentFactory::Instance().RegisterTypeInfo(TUniquePtr<TypeInfoBase>(new TypeInfo<cls>(#cls))) }

#if 0
	class Transform: public LevelComponent {
		REGISTER_LEVEL_COMPONENT(Transform);
	};
#endif
}