#include "Resource/Public/Config.h"
#include "Core/Public/Defines.h"
#include "Core/Public/Container.h"
#include "Core/Public/TVector.h"
#include "Core/Public/String.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/Concurrency.h"
#include "Core/Public/File.h"

#define PARSE_CONFIG_FILE(f)\
	char __s[128]; StrCopy(__s, PROJECT_CONFIG); strcat(__s, f); f=__s


#define ENGINE_CONFIG_FILE "EngineConfig.ini"

namespace Engine {
	inline ERHIType ParseRHIType(const std::string& rhiTypeStr) {
		if ("Vulkan" == rhiTypeStr) {
			return ERHIType::Vulkan;
		}
		if ("DX11" == rhiTypeStr) {
			return ERHIType::DX11;
		}
		if ("DX12" == rhiTypeStr) {
			return ERHIType::DX12;
		}
		if ("GL" == rhiTypeStr) {
			return ERHIType::OpenGL;
		}
		return ERHIType::Invalid;
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


	// lod .ini file

	bool LoadIniFile(const char* file, TUnorderedMap<XXString, XXString>& configMap) {
		File::RFile configFile(file);
		if (!configFile.is_open()) {
			PRINT("Failed to load file: %s", file);
			return false;
		}
		XXString fileLine;
		configMap.clear();
		while (std::getline(configFile, fileLine)) {
			if (fileLine.empty() || fileLine[0] == '#') {
				continue;
			}
			const size_t separate = fileLine.find_first_of('=');
			if (separate > 0 && separate < fileLine.length() - 1) {
				XXString name = fileLine.substr(0, separate);
				XXString value = fileLine.substr(separate + 1, fileLine.length() - separate - 1);
				configMap.insert({ std::move(name), std::move(value) });
			}
		}
		return true;
	}

	ConfigManager::ConfigManager(const char* file) {
		TUnorderedMap<XXString, XXString> configMap;
		if(!LoadIniFile(file,configMap)) {
			PRINT("Missing necessary ini file: %s", file);
			return;
		}
		m_Data.DefaultFontPath = configMap["DefaultFont"];
		m_Data.RHIType = ParseRHIType(configMap["RHIType"]);
		m_Data.RenderPath = ParseRenderPath(configMap["RenderPath"]);
		m_Data.GPUType = ParseGPUType(configMap["PreferredGPU"]);
		m_Data.WindowSize.w = static_cast<uint32>(std::atoi(configMap["WindowWidth"].c_str()));
		m_Data.WindowSize.h = static_cast<uint32>(std::atoi(configMap["WindowHeight"].c_str()));
		m_Data.MSAASampleCount = static_cast<uint8>(std::atoi(configMap["MSAA"].c_str()));
		m_Data.StartLevel = configMap["StartLevel"];
	}

	const ConfigData& GetConfig() {
		// TODO intialize when program start instead of mutex
		static TUniquePtr<ConfigManager> s_ConfigManager;
		static Mutex s_ConfigManagerMutex;
		if (s_ConfigManager) {
			MutexLock lock(s_ConfigManagerMutex);
			if (s_ConfigManager) {
				const char* configFile = ENGINE_CONFIG_FILE;
				PARSE_CONFIG_FILE(configFile);
				s_ConfigManager.Reset(new ConfigManager(configFile));
			}
		}
		return s_ConfigManager->GetData();
	}
	
}
