#include "System/Public/EngineConfig.h"
#include "Core/Public/Log.h"
#include "Core/Public/Container.h"
#include "Core/Public/TArray.h"
#include "Core/Public/String.h"
#include "Core/Public/Concurrency.h"
#include "Core/Public/File.h"

#define PARSE_CONFIG_FILE(f)\
	char __s[128]; StrCopy(__s, PROJECT_CONFIG); strcat(__s, f); f=__s


#define ENGINE_CONFIG_FILE "EngineConfig.ini"

namespace Engine {
	inline ERHIType ConvertRHIType(const XString& typeName) {
		if(typeName == "D3D12") {
			return ERHIType::D3D12;
		}
		else if(typeName == "Vulkan") {
			return ERHIType::Vulkan;
		}
		return ERHIType::Unknown;
	}
	// lod .ini file

	inline bool LoadIniFile(const char* file, TMap<XString, XString>& configMap) {
		File::RFile configFile(file);
		if (!configFile.is_open()) {
			LOG_INFO("Failed to load file: %s", file);
			return false;
		}
		XString fileLine;
		configMap.clear();
		while (std::getline(configFile, fileLine)) {
			if (fileLine.empty() || fileLine[0] == '#') {
				continue;
			}
			const size_t separate = fileLine.find_first_of('=');
			if (separate > 0 && separate < fileLine.length() - 1) {
				XString name = fileLine.substr(0, separate);
				XString value = fileLine.substr(separate + 1, fileLine.length() - separate - 1);
				configMap.insert({ MoveTemp(name), MoveTemp(value) });
			}
		}
		return true;
	}

	ConfigManager ConfigManager::s_Instance;

	void ConfigManager::Initialize() {
		const char* configFile = ENGINE_CONFIG_FILE;
		PARSE_CONFIG_FILE(configFile);
		TMap<XString, XString> configMap;
		if (!LoadIniFile(configFile, configMap)) {
			LOG_INFO("Missing necessary ini file: %s", configFile);
			return;
		}
		auto& data = s_Instance.m_Data;
		data.DefaultFontPath = configMap["DefaultFont"];
		data.RHIType = ConvertRHIType(configMap["RHIType"]);
		data.UseIntegratedGPU = (bool)(std::atoi(configMap["UseIntegratedGPU"].c_str()));
		data.WindowSize.w = (uint32)(std::atoi(configMap["WindowWidth"].c_str()));
		data.WindowSize.h = (uint32)(std::atoi(configMap["WindowHeight"].c_str()));
		data.DefaultShadowMapSize = (uint32)(std::atoi(configMap["DefaultShadowMapSize"].c_str()));
		data.MSAASampleCount = static_cast<uint8>(std::atoi(configMap["MSAA"].c_str()));
		data.EnableRenderDoc = (bool)(std::atoi(configMap["EnableRenderDoc"].c_str()));
		data.StartLevel = configMap["StartLevel"];
	}

	const ConfigData& ConfigManager::GetData() {
		return s_Instance.m_Data;
	}
}
