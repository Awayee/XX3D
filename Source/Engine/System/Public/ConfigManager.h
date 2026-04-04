#pragma once
#include "Core/Public/ConfigProperty.h"

namespace Engine{

	enum class ERHIType : uint8 {
		Unknown,
		Vulkan,
		D3D12,
	};

	struct XXEngineConfig {
		CONFIG_PROPERTY_BEING(XXEngineConfig)
		CONFIG_PROPERTY_STRING(Engine, DefaultFont, );
		CONFIG_PROPERTY_STRING(Engine, ProjectPath, );
		CONFIG_PROPERTY_END(XXEngineConfig)
	};

    struct XXProjectConfig {
		CONFIG_PROPERTY_BEING(XXProjectConfig)
		CONFIG_PROPERTY_STRING(Project, ProjectName, );
		CONFIG_PROPERTY_STRING(Project, StartLevel, );

		CONFIG_PROPERTY_STRING(Rendering, DefaultFont, );
		CONFIG_PROPERTY_UINT(Rendering, WindowWidth, 1280u);
		CONFIG_PROPERTY_UINT(Rendering, WindowHeight, 720);
		CONFIG_PROPERTY_UINT(Rendering, DefaultShadowMapSize, 1024);
		CONFIG_PROPERTY_ENUM(Rendering, ERHIType, RHIType, ERHIType::Vulkan);
		CONFIG_PROPERTY_INT(Rendering, MSAASampleCount, 1);
		CONFIG_PROPERTY_BOOL(Rendering, EnableRenderDoc, false);
		CONFIG_PROPERTY_INT(Rendering, UseIntegratedGPU, false);
		CONFIG_PROPERTY_BOOL(Rendering, EnableGPUDriven, false);
    	CONFIG_PROPERTY_END(XXProjectConfig)
    };

    class ConfigMgr {
    public:
        ConfigMgr();
        ~ConfigMgr();
		static ConfigMgr& Instance();
		static void Initialize();

		const XString& GetExecutableDir() const { return  ExecutableDir; }
		const XString& GetEngineAssetDir() const { return  EngineAssetDir; }
		const XString& GetShaderDir() const { return  ShaderDir; }
		const XString& GetDefaultFont() const { return EngineConfig.DefaultFont; }
		const XString& GetProjectDir() const { return EngineConfig.ProjectPath; }
		const XXProjectConfig& GetProjectConfig() { return  ProjectConfig; }
		XString GetCompiledShaderDir() const;
		XString GetProjectAssetDir() const;
    private:
		XXEngineConfig EngineConfig;
		XXProjectConfig ProjectConfig;

		XString EngineRoot;
		XString ExecutableDir;
		XString ShaderDir;
		XString EngineAssetDir;
    };
}