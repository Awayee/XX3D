#pragma once
#include "EditorUI/Public/EditorUI.h"
#include "Objects/Public/Engine.h"
#include "Core/Public/SmartPointer.h"
#include "Render/Public/RenderMesh.h"
#include "EditorAsset/Public/MeshImporter.h"

namespace Editor {
	class XXEditor {
	private:
		// TODO TEST
		Engine::RenderScene* m_MainScene;
		Engine::Camera* m_MainCamera;
		Engine::XXEngine* m_Engine;
		TUniquePtr<UIMgr> m_EditorUI;
		TUniquePtr<AMeshAsset> m_MeshAsset;
		TUniquePtr<Engine::RenderMesh> m_RenderMesh;

	private:
		void InitResources();
	public:
		XXEditor(Engine::XXEngine* engine);
		~XXEditor();
		void EditorRun();
	};
}
