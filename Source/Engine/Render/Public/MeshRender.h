#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/TUniquePtr.h"

namespace Render {
	RHIGraphicsPipelineStatePtr CreateMeshGBufferRenderPSO(TConstArrayView<ERHIFormat> colorFormats, ERHIFormat depthFormat);
	RHIGraphicsPipelineStatePtr CreateDeferredLightingPSO(ERHIFormat colorFormat);
}