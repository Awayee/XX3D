#include "VulkanContext.h"
#include "Core/Public/TVector.h"
#include "Core/Public/String.h"
#include "Core/Public/TArray.h"
#include <GLFW/glfw3.h>

static const char* VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

inline bool CheckLayerSupported(const TVector<const char*>& layers){
	uint32 layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	TVector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.Data());
	for (auto& name : layers){
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers){
			if (StrEqual(name, layerProperties.layerName)){
				layerFound = true;
				break;
			}
		}
		if (!layerFound){
			LOG_ERROR("layers is not supported! %s", name);
			return false;
		}
	}
	return true;
}

inline void GetInstanceExtensions(TVector<const char*>& extensions, bool enableDebug) {
	uint32 glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	extensions.Resize(glfwExtensionCount);
	memcpy(extensions.Data(), glfwExtensions, glfwExtensionCount * sizeof(const char*));
	if (enableDebug) {
		extensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
}

inline bool CheckInstanceExtensionsSupported(const TVector<const char*>& extensions) {
	uint32 extensionCount{ 0 };
	VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr), "");
	TVector<VkExtensionProperties> extensionProperties(extensionCount);
	VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.Data()), "");
	for (auto extension : extensions) {
		bool supported = false;
		for (auto& extensionProperty : extensionProperties) {
			if (StrEqual(extensionProperty.extensionName, extension)) {
				supported = true;
				break;
			}
		}
		if(!supported) {
			return false;
		}
	}
	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
	VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* data){
	LOG_ERROR("[Vulkan Error] %s", pCallbackData->pMessage);
	return VK_FALSE;
}

inline void SetupDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& debugInfo) {
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = DebugCallback;
}


VulkanContext::VulkanContext(bool enableDebug, uint32 apiVersion): m_APIVersion(apiVersion) {
	CreateInstance(enableDebug);
	if (enableDebug) {
		CreateDebugUtilsMessenger();
	}
}

VulkanContext::~VulkanContext() {
	if (!m_Instance) {
		return;
	}
	if (m_DebugUtilsMessenger) {
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugUtilsMessenger, nullptr);
	}
	vkDestroyInstance(m_Instance, nullptr);
}

VkPhysicalDevice VulkanContext::PickPhysicalDevice() {
	uint32 physicalDeviceCount;
	if(VK_SUCCESS != vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, nullptr)) {
		return VK_NULL_HANDLE;
	}
	TFixedArray<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	if(VK_SUCCESS != vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, physicalDevices.Data())) {
		return VK_NULL_HANDLE;
	}

	struct PhysicalDeviceInfo {
		uint32 Order;
		VkPhysicalDevice PhysicalDevice;
		VkPhysicalDeviceProperties Properties;
	};
	TVector<PhysicalDeviceInfo> deviceInfos;
	for(uint32 i=0; i<physicalDeviceCount; ++i) {

		// TODO device extensions check
		// TODO device features check
		PhysicalDeviceInfo& info = deviceInfos.EmplaceBack();
		info.Order = i;
		info.PhysicalDevice = physicalDevices[i];
		vkGetPhysicalDeviceProperties(info.PhysicalDevice, &info.Properties);
	}

	Engine::EGPUType gpuType = Engine::ConfigManager::GetData().GPUType;
	deviceInfos.Sort([gpuType](const PhysicalDeviceInfo& l, const PhysicalDeviceInfo& r)->bool {
		if(l.Properties.deviceType == r.Properties.deviceType) {
			return l.Order < r.Order;
		}
		if(gpuType == Engine::EGPUType::GPU_UNKNOWN || gpuType == Engine::EGPUType::GPU_DISCRETE) {
			return l.Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
				   r.Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
		}
		// integrated gpu
		return l.Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
			r.Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
	});

	for(const PhysicalDeviceInfo& info: deviceInfos) {
		if(info.Properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU) {
			return info.PhysicalDevice;
		}
	}
	return VK_NULL_HANDLE;
}

void VulkanContext::CreateInstance(bool enableDebug) {
	InitializeInstanceProcAddr();
	// Check vulkan API version
	if(!vkEnumerateInstanceVersion) {
		LOG_ERROR("Vulkan API version < 1.1 !");
		return;
	}
	uint32 supportedVersion;
	vkEnumerateInstanceVersion(&supportedVersion);
	LOG_INFO("Supported Vulkan API version: %i.%i.%i", VK_API_VERSION_MAJOR(supportedVersion), VK_API_VERSION_MINOR(supportedVersion), VK_API_VERSION_PATCH(supportedVersion) );
	if(supportedVersion < m_APIVersion) {
		LOG_ERROR("Supported Vulkan API Version < MIN_API_VERSION");
		return;
	}
	if(supportedVersion > m_APIVersion) {
		m_APIVersion = supportedVersion;
	}

	// Check extensions
	TVector<const char*> extensions;
	GetInstanceExtensions(extensions, enableDebug);
	if(!CheckInstanceExtensionsSupported(extensions)) {
		LOG_ERROR("Instance extension is not supported!");
		return;
	}

	// app info
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = PROJECT_NAME;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = ENGINE_NAME;
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = m_APIVersion;

	// create info
	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0};
	createInfo.pApplicationInfo = &appInfo; // the appInfo is stored here
	createInfo.enabledExtensionCount = extensions.Size();
	createInfo.ppEnabledExtensionNames = extensions.Data();
	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	const TVector<const char*> layers{ VALIDATION_LAYER_NAME };
	if (enableDebug) {
		if (CheckLayerSupported(layers)) {
			createInfo.enabledLayerCount = layers.Size();
			createInfo.ppEnabledLayerNames = layers.Data();
			SetupDebugInfo(debugCreateInfo);
			debugCreateInfo.pUserData = (void*)this;
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			LOG_WARNING("[VulkanContext::CreateInstance] Validation layer is not supported!");
		}
	}
	VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &m_Instance), "vkCreateInstance");
	LOG_INFO("Instance Created!");
	LoadInstanceFunctions(m_Instance);
}

void VulkanContext::CreateDebugUtilsMessenger() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	SetupDebugInfo(createInfo);
	createInfo.pUserData = (void*)this;
	VK_ASSERT(vkCreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugUtilsMessenger), "");
	LOG_INFO("Debug utils messenger created!");
}
