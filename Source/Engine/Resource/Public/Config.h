#pragma once
#include "Core/Public/File.h"
#include "Core/Public/BaseStructs.h"
#include "RHI/Public/RHI.h"
namespace Engine {

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
		XXString      DefaultFontPath;
		EGPUType      GPUType;
		ERHIType      RHIType;
		uint32        MSAASampleCount{ 0 };
		ERenderPath   RenderPath;
		USize2D       WindowSize;
		XXString      StartLevel;
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
