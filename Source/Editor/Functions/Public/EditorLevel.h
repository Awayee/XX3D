#pragma once
#include "Objects/Public/Level.h"

namespace Editor {
	class EditorLevel: public Object::Level{
	public:
		using Level::Level;
		bool LoadFile(const char* file);
		bool SaveFile(const char* file);
		uint32 GetActorSize();
		Object::LevelActor* GetActor(uint32 actorID);
	};

	class EditorLevelMgr {
		SINGLETON_INSTANCE(EditorLevelMgr);
	public:
		void LoadLevel(File::PathStr levelFile);
		void SetLevelPath(File::PathStr levelFile);
		void ReloadLevel();
		bool SaveLevel();
		const XString& GetLevelFile() { return m_LevelFile; }
		EditorLevel* GetLevel() { return m_Level.Get(); }
		void SetSelected(uint32 idx) { m_SelectIndex = idx; }
		uint32 GetSelected() const { return m_SelectIndex; }
	private:
		XString m_LevelFile;
		TUniquePtr<EditorLevel> m_Level;
		uint32 m_SelectIndex{ UINT32_MAX };
		EditorLevelMgr();
		~EditorLevelMgr() = default;
	};

	// type system
	class LevelComponentEditProxyFactory {
	public:
		struct LevelComponentEditProxy {
			Func<void(Object::LevelComponent*, Json::ValueWriter&)> OnSave{ nullptr };
			Func<void(Object::LevelComponent*)> OnGUI{ nullptr };
		};
		template<class T>
		bool RegisterOnSave(void(*func)(T*, Json::ValueWriter& val)) {
			m_InitializerArray.PushBack([func](LevelComponentEditProxyFactory* f) {
				f->GetProxy(T::TypeID).OnSave = [func](Object::LevelComponent* c, Json::ValueWriter& v) { func((T*)c, v); };
				});
			return true;
		}
		template<class T>
		bool RegisterOnGUI(void(*func)(T*)) {
			m_InitializerArray.PushBack([func](LevelComponentEditProxyFactory* f) {
				f->GetProxy(T::TypeID).OnGUI = [func](Object::LevelComponent* c) { func((T*)c); };
				});
			return true;
		}

		void Initialize();
		const LevelComponentEditProxy& GetProxy(Object::LevelComponent* c);
		static LevelComponentEditProxyFactory& Instance();
	private:
		TArray<Func<void(LevelComponentEditProxyFactory*)>> m_InitializerArray;
		TArray<LevelComponentEditProxy> m_EditProxies;
		LevelComponentEditProxyFactory() = default;
		~LevelComponentEditProxyFactory() = default;
		LevelComponentEditProxy& GetProxy(uint32 typeID);
	};
#define REGISTER_LEVEL_EDIT_OnSave(cls, func) namespace{static bool s_##func = LevelComponentEditProxyFactory::Instance().RegisterOnSave<cls>(func);}
#define REGISTER_LEVEL_EDIT_OnGUI(cls, func) namespace{static bool s_##func = LevelComponentEditProxyFactory::Instance().RegisterOnGUI<cls>(func);}
}