#pragma once
#include "Core/Public/File.h"
#include "Core/Public/BaseStructs.h"
namespace Engine {
	enum ERHIType {
		RHI_Vulkan,
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

	class ConfigManager {
	private:
		String m_DefaultFontPath;
		ERHIType m_RHIType;
		ERenderPath m_RenderPath;
		EGPUType m_GpuType;
		USize2D m_WindowSize;
	public:
		ConfigManager(const char* configPath);
		~ConfigManager();
		const String& GetDefaultFontPath() { return m_DefaultFontPath; }
		ERHIType GetRHIType() { return m_RHIType; }
		ERenderPath GetRenderPath() { return m_RenderPath; }
		EGPUType GetGPUType() { return m_GpuType; }
		const USize2D& GetWindowSize() { return m_WindowSize; }
	};

	ConfigManager* GetConfig();	
}
