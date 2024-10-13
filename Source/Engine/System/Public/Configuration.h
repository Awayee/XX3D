#pragma once
#include "Core/Public/File.h"
#include "System/Public/Configuration.h"
namespace Engine {
	enum class ERHIType : uint8 {
		D3D12,
		Vulkan,
		Unknown
	};

	class EngineConfig {
	public:
		File::FPath GetExecutableDir() const { return m_ExecutableDir; }
		File::FPath GetShaderDir() const { return m_ShaderDir; }
		File::FPath GetCompiledShaderDir() const;
		File::FPath GetEngineAssetDir() const { return m_EngineAssetDir; }
		File::FPath GetProjectDir() const { return m_ProjectDir; }
		File::FPath GetProjectAssetDir() const;
		const XString& GetDefaultFont() const { return m_DefaultFont; }
		static const EngineConfig& Instance();
		static void Initialize();
	private:
		File::FPath m_ExecutableDir;
		File::FPath m_ShaderDir;
		File::FPath m_EngineAssetDir;
		File::FPath m_ProjectDir;
		XString     m_DefaultFont;
		EngineConfig();
		~EngineConfig() = default;
	};

	class ProjectConfig {
	public:
		XString  ProjectName;
		XString  StartLevel;
		uint32   WindowWidth{1280};
		uint32   WindowHeight{720};
		uint32   DefaultShadowMapSize{1024};
		ERHIType RHIType{ERHIType::D3D12};
		uint8    MSAASampleCount{ 1 };
		bool     EnableRenderDoc {false};
		bool     UseIntegratedGPU{false};
		static const ProjectConfig& Instance();
		static void Initialize();
	private:
		ProjectConfig();
		~ProjectConfig() = default;
	};
}