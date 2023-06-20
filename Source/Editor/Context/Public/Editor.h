#pragma once
#include "EditorUI/Public/EditorUI.h"
#include "Objects/Public/Engine.h"
#include "Core/Public/SmartPointer.h"

#include "Render/Public/StaticMesh.h"

#include "EditorAsset/Public/TextureImporter.h"
#include "EditorAsset/Public/MeshImporter.h"

namespace Editor {
	class XXEditor {
	private:
		Engine::XXEngine* m_Engine;
		TUniquePtr<UIMgr> m_EditorUI;

	public:
		XXEditor(Engine::XXEngine* engine);
		~XXEditor();
		void EditorRun();
	};
}
