#include <memory>
#include <string>
#include <mutex>
#include "Resource/Public/Config.h"
#include "Core/Public/macro.h"
#include "Core/Public/Container.h"

#define PARSE_CONFIG_FILE(f)\
	char __s[128]; strcpy(__s, PROJECT_CONFIG); strcat(__s, f); f=__s


#define ENGINE_CONFIG_FILE "EngineConfig.ini"

namespace Engine {
	inline ERHIType ParseRHIType(const std::string& rhiTypeStr) {
		if ("Vulkan" == rhiTypeStr) {
			return RHI_VULKAN;
		}
		if ("DX11" == rhiTypeStr) {
			return RHI_DX11;
		}
		if ("DX12" == rhiTypeStr) {
			return RHI_DX12;
		}
		if ("GL" == rhiTypeStr) {
			return RHI_GL;
		}
		return RHI_NONE;
	}

	inline ERenderPath ParseRenderPath(const std::string& renderPathStr) {
		if ("Deferred" == renderPathStr) {
			return RENDER_DEFERRED;
		}
		if ("Forward" == renderPathStr) {
			return RENDER_FORWARD;
		}
		return RENDER_NONE;
	}

	inline EGPUType ParseGPUType(const std::string& gpuType) {
		if ("Integrated" == gpuType) {
			return GPU_INTEGRATED;
		}
		if ("Discrete" == gpuType) {
			return GPU_DISCRETE;
		}
		return GPU_NONE;
	}

	ConfigManager::ConfigManager(const char* file) {
		TUnorderedMap<String, String> configMap;
		ASSERT(File::LoadIniFile(file, configMap), "");
		m_Data.DefaultFontPath = configMap["DefaultFont"];
		m_Data.RHIType = ParseRHIType(configMap["RHIType"]);
		m_Data.RenderPath = ParseRenderPath(configMap["RenderPath"]);
		m_Data.GPUType = ParseGPUType(configMap["PreferredGPU"]);
		m_Data.WindowSize.w = UINT32_CAST(std::atoi(configMap["WindowWidth"].c_str()));
		m_Data.WindowSize.h = UINT32_CAST(std::atoi(configMap["WindowHeight"].c_str()));
		m_Data.MSAASampleCount = UINT8_CAST(std::atoi(configMap["MSAA"].c_str()));
	}

	const ConfigData& GetConfig() {
		static std::unique_ptr<ConfigManager> s_ConfigManager;
		static std::mutex s_ConfigManagerMutex;
		if (nullptr == s_ConfigManager) {
			std::lock_guard<std::mutex> lock(s_ConfigManagerMutex);
			if (nullptr == s_ConfigManager) {
				const char* configFile = ENGINE_CONFIG_FILE;
				PARSE_CONFIG_FILE(configFile);
				s_ConfigManager.reset(new ConfigManager(configFile));
			}
		}
		return s_ConfigManager->GetData();
	}
	
}
