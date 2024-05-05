#pragma once
#include <d3d12.h>
#include "RHI/Public/RHIResources.h"

namespace Engine {
	class RHIBufferDX12: public RHIBuffer {
		ID3D12Resource* m_Resource;
	};
}
