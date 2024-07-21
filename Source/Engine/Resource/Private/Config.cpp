#include "Resource/Public/Config.h"
#include "Core/Public/Log.h"
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
			return ERenderPath::RENDER_DEFERRED;
		}
		if ("Forward" == renderPathStr) {
			return ERenderPath::RENDER_FORWARD;
		}
		return ERenderPath::RENDER_NONE;
	}

	inline EGPUType ParseGPUType(const std::string& gpuType) {
		if ("Integrated" == gpuType) {
			return EGPUType::GPU_INTEGRATED;
		}
		if ("Discrete" == gpuType) {
			return EGPUType::GPU_DISCRETE;
		}
		return EGPUType::GPU_UNKNOWN;
	}


	// lod .ini file

	bool LoadIniFile(const char* file, TMap<XXString, XXString>& configMap) {
		File::RFile configFile(file);
		if (!configFile.is_open()) {
			LOG_INFO("Failed to load file: %s", file);
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
				configMap.insert({ MoveTemp(name), MoveTemp(value) });
			}
		}
		return true;
	}

	ConfigManager ConfigManager::s_Instance;

	void ConfigManager::Initialize() {
		const char* configFile = ENGINE_CONFIG_FILE;
		PARSE_CONFIG_FILE(configFile);
		TMap<XXString, XXString> configMap;
		if (!LoadIniFile(configFile, configMap)) {
			LOG_INFO("Missing necessary ini file: %s", configFile);
			return;
		}
		auto& data = s_Instance.m_Data;
		data.DefaultFontPath = configMap["DefaultFont"];
		data.RHIType = ParseRHIType(configMap["RHIType"]);
		data.RenderPath = ParseRenderPath(configMap["RenderPath"]);
		data.GPUType = ParseGPUType(configMap["PreferredGPU"]);
		data.WindowSize.w = static_cast<uint32>(std::atoi(configMap["WindowWidth"].c_str()));
		data.WindowSize.h = static_cast<uint32>(std::atoi(configMap["WindowHeight"].c_str()));
		data.MSAASampleCount = static_cast<uint8>(std::atoi(configMap["MSAA"].c_str()));
		data.StartLevel = configMap["StartLevel"];
	}

	const ConfigData& ConfigManager::GetData() {
		return s_Instance.m_Data;
	}
}
