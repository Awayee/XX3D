#include "RHI/Public/RHI.h"
#include "RHIVulkan/RHIVulkan.h"
#include "RHID3D11/RHID3D11.h"
#include "Resource/Public/Config.h"
#include "Core/Public/macro.h"
#include "Core/Public/TPtr.h"
#include "Core/Public/Concurrency.h"

namespace Engine{

	RHI* RHI::Instance() {
		static TUniquePtr<RHI> s_Instance;
		static Mutex s_InstanceMutex;

		if(nullptr == s_Instance) {
			MutexLock lock(s_InstanceMutex);
			if(nullptr == s_Instance) {
				ERHIType rhiType = GetConfig().RHIType;
				if(RHI_VULKAN == rhiType) {
					s_Instance.reset(new RHIVulkan());
				}
				else if (RHI_DX11 == rhiType) {
					//s_Instance.reset(new RHIDX11());
				}
				else {
					ERROR("Failed to initialize RHI!");
				}
			}
		}

		return s_Instance.get();
	}
}
