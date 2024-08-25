#pragma once
#include "Core/Public/File.h"
#include "Core/Public/BaseStructs.h"
#include "RHI/Public/RHI.h"
namespace Engine {

	enum class ERenderPath {
		RENDER_FORWARD,
		RENDER_DEFERRED,
		RENDER_NONE
	};

	enum class EGPUType {
		GPU_INTEGRATED,
		GPU_DISCRETE,
		GPU_UNKNOWN
	};
	enum class ERHIType {
		Vulkan,
		DX12,
		DX11,
		OpenGL,
		Invalid
	};

	struct ConfigData {
		XString      DefaultFontPath;
		EGPUType      GPUType;
		ERHIType      RHIType;
		uint32        MSAASampleCount{ 0 };
		ERenderPath   RenderPath;
		USize2D       WindowSize;
		uint32        DefaultShadowMapSize;
		XString       StartLevel;
	};

	class ConfigManager {
	public:
		static void Initialize();
		static const ConfigData& GetData();
	private:
		friend class ConfigModule;
		static ConfigManager s_Instance;
		ConfigManager() = default;
		~ConfigManager() = default;
		ConfigData m_Data;
	};
}
