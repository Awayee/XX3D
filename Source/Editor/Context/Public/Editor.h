#pragma once
#include "EditorUI/Public/EditorUI.h"
#include "Objects/Public/Engine.h"
#include "Core/Public/SmartPointer.h"
#include "Render/Public/RenderMesh.h"

namespace Editor {
	class XXEditor {
	private:
		// TODO TEST
		TUniquePtr<Engine::RenderMesh> m_Ms;
		Engine::RenderScene* m_MainScene;
		Engine::Camera* m_MainCamera;

		Engine::XXEngine* m_Engine;
		TUniquePtr<UIMgr> m_EditorUI;

	private:
		void InitResources();
	public:
		XXEditor(Engine::XXEngine* engine);
		~XXEditor();
		void EditorRun();
	};
}
