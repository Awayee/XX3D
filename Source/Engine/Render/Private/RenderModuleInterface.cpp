#include "Render/Public/RenderModuleInterface.h"
#include "Render/Public/DefaultResource.h"
#include "Render/Public/Renderer.h"

namespace Render {

	void Initialize() {
		DefaultResources::Initialize();
		RendererMgr::Initialize();
	}

	void Update() {
		RendererMgr::Instance()->Update();
	}

	void Release() {
		DefaultResources::Release();
		RendererMgr::Release();
	}
}
