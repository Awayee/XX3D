#include "VulkanCommon.h"
#include "Core/Public/Log.h"

#define DEFINE_VULKAN_FUNCTION(type, name) type name{nullptr};
ENUM_VULKAN_LOADER_FUNCTIONS(DEFINE_VULKAN_FUNCTION)\
ENUM_VULKAN_INSTANCE_FUNCTIONS(DEFINE_VULKAN_FUNCTION)\
ENUM_VULKAN_DEVICE_FUNCTIONS(DEFINE_VULKAN_FUNCTION)\


#include <Windows.h>
static const char* VK_LIBRARY = "vulkan-1.dll";

bool InitializeInstanceProcAddr() {
	static HMODULE s_Lib = LoadLibraryA(VK_LIBRARY);
	if (s_Lib == nullptr) {
		return false;
	}
	vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(s_Lib, "vkGetInstanceProcAddr"));
#define LOAD_GLOBAL_PROC_ADDR(type, name) name = reinterpret_cast<type>(vkGetInstanceProcAddr(nullptr, #name));
	ENUM_VULKAN_LOADER_FUNCTIONS(LOAD_GLOBAL_PROC_ADDR);
	return true;
}

void LoadInstanceFunctions(VkInstance instance) {
#define LOAD_INSTANCE_PROC_ADDR(type, name) name = reinterpret_cast<type>(vkGetInstanceProcAddr(instance, #name));
	ENUM_VULKAN_INSTANCE_FUNCTIONS(LOAD_INSTANCE_PROC_ADDR);
}

void LoadDeviceFunctions(VkDevice device) {
#define LOAD_DEVICE_PROC_ADDR(type, name) \
	name = reinterpret_cast<type>(vkGetDeviceProcAddr(device, #name));\
	if(!(name)) {LOG_WARNING("Failed to load vulkan device funcion: "#name);}

	ENUM_VULKAN_DEVICE_FUNCTIONS(LOAD_DEVICE_PROC_ADDR);
}
