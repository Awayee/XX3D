#include "RHI/Public/RHI.h"
#include "RHIVulkan/RHIVulkan.h"
#include "Resource/Public/Config.h"
#include "Core/Public/macro.h"
#include "Core/Public/SmartPointer.h"
#include "Core/Public/Concurrency.h"

using namespace Engine;
namespace RHI{

	RHIInstance* GetInstance() {
		static TUniquePtr<RHIInstance> s_Instance;
		static Mutex s_InstanceMutex;

		if(nullptr == s_Instance) {
			MutexLock lock(s_InstanceMutex);
			if(nullptr == s_Instance) {
				ERHIType rhiType = GetConfig()->GetRHIType();
				if(RHI_Vulkan == rhiType) {
					s_Instance.reset(new RHIVulkan());
				}
				else {
					ERROR("Failed to initialize RHI!");
				}
			}
		}

		return s_Instance.get();
	}
}
