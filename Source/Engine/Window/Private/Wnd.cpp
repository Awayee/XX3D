#include "Window/Public/Wnd.h"
#include "Core/Public/Concurrency.h"
#include "Core/Public/macro.h"
#include "Core/Public/TPtr.h"
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
				if(RHI_VULKAN == rhiType) {
					s_Instance.reset(new WindowSystemGLFW);
				}
				else if(RHI_DX11 == rhiType) {
					s_Instance.reset(new WindowSystemWin32);
				}
				else {
					ASSERT(0, "can not resolve rhi type");
				}
			}
		}
		return s_Instance.get();
	}
}
