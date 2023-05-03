#pragma once
#include "Core/Public/File.h"
#include "Core/Public/BaseStructs.h"
namespace Engine {
	enum ERHIType {
		RHI_VULKAN,
		RHI_DX11,
		RHI_DX12,
		RHI_GL,
		RHI_NONE
	};

	enum ERenderPath {
		RENDER_FORWARD,
		RENDER_DEFERRED,
		RENDER_NONE
	};

	enum EGPUType {
		GPU_INTEGRATED,
		GPU_DISCRETE,
		GPU_NONE
	};

	struct ConfigData {
		String        DefaultFontPath;
		EGPUType      GPUType;
		ERHIType      RHIType;
		uint32        MSAASampleCount{ 0 };
		ERenderPath   RenderPath;
		USize2D       WindowSize;
	};

	class ConfigManager {
	private:
		ConfigData m_Data;
	public:
		const ConfigData& GetData() { return m_Data; }
		ConfigManager(const char* configPath);
		~ConfigManager()=default;
	};

	const ConfigData& GetConfig();
}
