#pragma once
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Core/Public/Json.h"

namespace Object {
	class LevelComponentContainer;
	class LevelActor;
	class Level;

	class LevelComponentBase {
	public:
		virtual ~LevelComponentBase() = default;
		virtual void OnLoad(const Json::Value& val) = 0;
		virtual void OnAdd() {/*Do nothing*/ }
		virtual void OnRemove() {/*Do nothing*/ }
		virtual uint32 GetTypeID() = 0;
		template<class T> bool Is() { return T::TypeID == GetTypeID(); }
	};

	// LevelComponentContainer must be used with derived class, so has not virtual destructor.
	class LevelComponentContainer {
	public:
		LevelComponentContainer() = default;
		~LevelComponentContainer() = default;
		template<class T> T* GetComponent() {
			for(auto& com: m_Components) {
				if(com->Is<T>()) {
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

		TArrayView<LevelComponentBase*> GetComponents();

		void RemoveComponent(LevelComponentBase* component);

	protected:
		TArray<TUniquePtr<LevelComponentBase>> m_Components;
	};

	class ActorComponent: public LevelComponentBase {
	public:
		explicit ActorComponent(LevelComponentContainer* actor);
		LevelActor* GetActor() { return m_Actor; }
		RenderScene* GetScene();
		uint32 GetEntityID();
	private:
		LevelActor* m_Actor;
	};

	class LevelComponent: public LevelComponentBase {
	public:
		explicit LevelComponent(LevelComponentContainer* level);
		Level* GetLevel() { return m_Level; }
		RenderScene* GetScene();
	private:
		Level* m_Level;
	};

	class LevelActor: public LevelComponentContainer {
	public:
		LevelActor(Level* level, XString&& name);
		~LevelActor();
		const XString& GetName() const { return m_Name; }
		void SetName(const XString& name) { m_Name = name; }
		Object::EntityID GetEntityID() const { return m_EntityID; }
		Object::RenderScene* GetScene() const;
	private:
		friend Level;
		static constexpr uint32 INVALID_INDEX{ UINT32_MAX };
		Level* m_Level;
		XString m_Name;
		Object::EntityID m_EntityID;
		uint32 m_ArrayIndex{ INVALID_INDEX };
	};

	class Level : public LevelComponentContainer{
	public:
		Level(RenderScene* scene);
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
		virtual LevelComponentBase* AddComponent(LevelComponentContainer* owner) = 0;
		const XString& GetTypeName() { return m_TypeName; }
	private:
		XString m_TypeName;
	};

	template<class T> class TypeInfo: public TypeInfoBase {
	public:
		using TypeInfoBase::TypeInfoBase;
		LevelComponentBase* AddComponent(LevelComponentContainer* owner) override {
			return owner->AddComponent<T>();
		}
	};

	class LevelComponentFactory {
	public:
		static LevelComponentFactory& Instance();
		uint32 RegisterTypeInfo(TUniquePtr<TypeInfoBase>&& typeInfoPtr);
		uint32 GetTypeSize();
		TypeInfoBase* GetTypeInfo(uint32 typeID);
		TypeInfoBase* GetTypeInfo(const char* name);
		TypeInfoBase* GetTypeInfo(LevelComponentBase* component);
	private:
		TArray<TUniquePtr<TypeInfoBase>> m_TypeInfos;
		TMap<XString, uint32> m_MapTypeNameID;
		NON_COPYABLE(LevelComponentFactory);
		NON_MOVEABLE(LevelComponentFactory);
		LevelComponentFactory() = default;
		~LevelComponentFactory() = default;
	};
#define REGISTER_ACTOR_COMPONENT(cls) public:\
	using ActorComponent::ActorComponent;\
	uint32 GetTypeID() override { return TypeID; }\
	inline static uint32 TypeID{ LevelComponentFactory::Instance().RegisterTypeInfo(TUniquePtr<TypeInfoBase>(new TypeInfo<cls>(#cls))) }


#define REGISTER_LEVEL_COMPONENT(cls) public:\
	using LevelComponent::LevelComponent;\
	uint32 GetTypeID() override { return TypeID; }\
	inline static uint32 TypeID{ LevelComponentFactory::Instance().RegisterTypeInfo(TUniquePtr<TypeInfoBase>(new TypeInfo<cls>(#cls))) }
#if 0
	class Transform: public Object::ActorComponent {
		REGISTER_ACTOR_COMPONENT(Transform);
	};

	class Camera: public Object::LevelComponent {
		REGISTER_LEVEL_COMPONENT(camera);
	};

#endif
}