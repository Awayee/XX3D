#include "System/Public/ConfigManager.h"

#define SHADER_COMPILED_DIR_NAME "CompiledShaders"
#define ASSET_DIR_NAME "Assets"
#define SHADER_DIR_NAME "Shaders"
#define ENGINE_CONFIG_FILENAME "EngineConfig.ini"
#define PROJECT_CONFIG_FILENAME "ProjectConfig.ini"
#define CACHE_NAME "Cache"

namespace Engine {
	ConfigMgr::ConfigMgr() {
		File::FPath ExecutablePath = File::GetExecutablePath();
		File::FPath ParentPath = ExecutablePath.parent_path();
		// shader path
		ShaderDir = File::FPath(ParentPath).append(SHADER_DIR_NAME).string();
		// engine asset path
		EngineAssetDir = File::FPath(ParentPath).append(ASSET_DIR_NAME).string();
		EngineRoot = ParentPath.string();
		ExecutableDir = ExecutablePath.string();
		LOG_DEBUG("EngineRoot=%s", EngineRoot.c_str());
		// Ensure cache dir exists
		File::FPath EngineCachePath = ExecutablePath.append(CACHE_NAME);
		EngineCacheDir = EngineCachePath.string();

		// load EngineConfig.ini
		const XString EngineConfigFile = File::FPath(ParentPath).append(ENGINE_CONFIG_FILENAME).string();
		XXIniParser EngineIni;
		if (!EngineIni.ReadFile(EngineConfigFile)) {
			LOG_ERROR("Failed to load engine config file %s", EngineConfigFile.c_str());
			return;
		}
		EngineConfig.LoadConfig(EngineIni);

		// Resolve project path
		File::FPath ProjectPath = EngineConfig.ProjectPath;
		if(!ProjectPath.is_absolute()) {
			ProjectPath = ParentPath / ProjectPath;
			EngineConfig.ProjectPath = ProjectPath.string();
		}

		CHECK(File::Exist(EngineConfig.ProjectPath));
		LOG_DEBUG("ProjectDir=%s", EngineConfig.ProjectPath.c_str());

		// Load ProjectConfig.ini
		const XString ProjectConfigFile = File::CombinePathStr(GetProjectDir(), PROJECT_CONFIG_FILENAME);
		XXIniParser ProjectIni;
		if(!ProjectIni.ReadFile(ProjectConfigFile)) {
			LOG_ERROR("Failed to load project config fine: %s", ProjectConfigFile.c_str());
			return;
		}
		ProjectConfig.LoadConfig(ProjectIni);
	}

	ConfigMgr::~ConfigMgr() {
	}

	ConfigMgr& ConfigMgr::Instance() {
		static ConfigMgr s_Instance;
		return s_Instance;
	}

	void ConfigMgr::Initialize() {
		Instance();
	}
}
