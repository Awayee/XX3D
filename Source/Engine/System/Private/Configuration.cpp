#include "System/Public/Configuration.h"
#include "Core/Public/Container.h"

#define SHADER_COMPILED_DIR_NAME "Compiled"
#define ASSET_DIR_NAME "Assets"
#define SHADER_DIR_NAME "Shaders"
#define ENGINE_CONFIG_FILENAME "EngineConfig.ini"
#define PROJECT_CONFIG_FILENAME "ProjectConfig.ini"

namespace Engine {

	// lod .ini file
	inline bool LoadIniFile(const char* file, TMap<XString, XString>& configMap) {
		File::ReadFile configFile(file, false);
		if (!configFile.IsOpen()) {
			LOG_INFO("Failed to load file: %s", file);
			return false;
		}
		XString fileLine;
		configMap.clear();
		while (configFile.GetLine(fileLine)) {
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
	inline ERHIType ConvertRHIType(const XString& typeName) {
		if (typeName == "D3D12") {
			return ERHIType::D3D12;
		}
		else if (typeName == "Vulkan") {
			return ERHIType::Vulkan;
		}
		return ERHIType::Unknown;
	}

	File::FPath EngineConfig::GetCompiledShaderDir() const {
		return GetShaderDir().append(SHADER_COMPILED_DIR_NAME);
	}

	File::FPath EngineConfig::GetProjectAssetDir() const {
		return GetProjectDir().append(ASSET_DIR_NAME);
	}

	const EngineConfig& EngineConfig::Instance() {
		static EngineConfig s_Instance;
		return s_Instance;
	}

	void EngineConfig::Initialize() {
		Instance();
	}

	EngineConfig::EngineConfig() {
		m_ExecutableDir = File::GetExecutablePath();
		File::FPath parentPath = m_ExecutableDir.parent_path();
		// shader path
		m_ShaderDir = File::FPath(parentPath).append(SHADER_DIR_NAME);
		// engine asset path
		m_EngineAssetDir = File::FPath(parentPath).append(ASSET_DIR_NAME);
		// load EngineConfig.ini
		const XString engineConfigFile = File::FPath(parentPath).append(ENGINE_CONFIG_FILENAME).string();
		TMap<XString, XString> configMap;
		if(!LoadIniFile(engineConfigFile.c_str(), configMap)) {
			LOG_WARNING("Missing engine config file %s", engineConfigFile.c_str());
			return;
		}
		// project path
		m_ProjectDir = configMap["ProjectPath"];
		// default font
		m_DefaultFont = configMap["DefaultFont"];
	}

	void ProjectConfig::Initialize() {
		Instance();
	}

	const ProjectConfig& ProjectConfig::Instance() {
		static ProjectConfig s_Instance;
		return s_Instance;
	}

	ProjectConfig::ProjectConfig() {
		const XString configFile = EngineConfig::Instance().GetProjectDir().append(PROJECT_CONFIG_FILENAME).string();
		TMap<XString, XString> configMap;
		if (!LoadIniFile(configFile.c_str(), configMap)) {
			LOG_WARNING("Missing project config file: %s", configFile.c_str());
			return;
		}
		ProjectName = configMap["ProjectName"];
		StartLevel = configMap["StartLevel"];
		WindowWidth = StringToNum<uint32>(configMap["WindowWidth"].c_str());
		WindowHeight = StringToNum<uint32>(configMap["WindowHeight"].c_str());
		DefaultShadowMapSize = StringToNum<uint32>(configMap["DefaultShadowMapSize"].c_str());
		RHIType = ConvertRHIType(configMap["RHIType"]);
		MSAASampleCount = StringToNum<uint8>(configMap["MSAA"].c_str());
		UseIntegratedGPU = StringToNum<bool>(configMap["UseIntegratedGPU"].c_str());
		EnableRenderDoc = StringToNum<bool>(configMap["EnableRenderDoc"].c_str());
	}
}
