#include "Render/Public/RenderModuleInterface.h"
#include "Render/Public/Textures.h"
#include "Render/Public/Samplers.h"

namespace Render {

	void Initialize() {
		DefaultTexture::Initialize();
		DefaultSampler::Initialize();
	}

	void Update() {
	}

	void Release() {
		DefaultTexture::Release();
		DefaultSampler::Release();
	}
}
