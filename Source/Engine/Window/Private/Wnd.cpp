#include "Window/Public/Wnd.h"
#include "Core/Public/Concurrency.h"
#include "Core/Public/Defines.h"
#include "Core/Public/TUniquePtr.h"
#include "Resource/Public/Config.h"
#include "Window/Private/WindowGLFW/WindowSystemGLFW.h"
#include "WIndow/Private/WindowWin32/WindowSystemWin32.h"

namespace Engine {

	Wnd* Wnd::Instance() {
		static TUniquePtr<Wnd> s_Instance;
		static Mutex s_Mutex;
		if(!s_Instance) {
			MutexLock lock(s_Mutex);
			if(!s_Instance) {
				ERHIType rhiType = Engine::GetConfig().RHIType;
				if(ERHIType::Vulkan == rhiType) {
					s_Instance.Reset(new WindowSystemGLFW);
				}
				else if(ERHIType::Vulkan == rhiType) {
					s_Instance.Reset(new WindowSystemWin32);
				}
				else {
					ASSERT(0, "can not resolve rhi type");
				}
			}
		}
		return s_Instance.Get();
	}
}
