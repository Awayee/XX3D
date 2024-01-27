#include "RHI/Public/RHI.h"
#include "RHIVulkan/RHIVulkan.h"
#include "RHID3D11/RHID3D11.h"
#include "Resource/Public/Config.h"
#include "Core/Public/Defines.h"
#include "Core/Public/Concurrency.h"

RHI* RHI::s_Instance = nullptr;

RHI* RHI::Instance() {
	return s_Instance;
}

void RHI::Initialize(const RHIInitDesc& desc) {

	if (!s_Instance) {
		switch(desc.RHIType) {
		case ERHIType::Vulkan:
			s_Instance = new RHIVulkan(desc);
			break;
		case ERHIType::DX12:
		case ERHIType::DX11:
		case ERHIType::OpenGL:
		case ERHIType::Invalid:
			ASSERT(0, "Failed to initialize RHI!");
		}
	}
}

void RHI::Release() {
	if(s_Instance) {
		delete s_Instance;
	}
}
