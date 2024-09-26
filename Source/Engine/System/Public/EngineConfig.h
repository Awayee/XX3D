#pragma once
#include "Core/Public/File.h"
#include "Core/Public/BaseStructs.h"
#include "RHI/Public/RHI.h"
namespace Engine {
	enum class ERHIType: uint8 {
		D3D12,
		Vulkan,
		Unknown
	};
	struct ConfigData {
		XString       DefaultFontPath;
		ERHIType      RHIType;
		uint32        MSAASampleCount{ 0 };
		USize2D       WindowSize;
		uint32        DefaultShadowMapSize;
		XString       StartLevel;
		bool          EnableRenderDoc : 1;
		bool          UseIntegratedGPU : 1;
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
