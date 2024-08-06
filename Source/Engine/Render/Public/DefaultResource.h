#pragma once
#include "Core/Public/TSingleton.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHI.h"

namespace Render {
	class DefaultResources: public TSingleton<DefaultResources> {
	public:
		enum DefaultTextureType: uint32 {
			TEX_WHITE=0,
			TEX_BLACK,
			TEX_GRAY,
			TEX_MAX_NUM
		};
		RHITexture* GetDefaultTexture2D(DefaultTextureType type);
		RHITexture* GetDefaultTextureCube(DefaultTextureType type);
		RHISampler* GetDefaultSampler(ESamplerFilter filter, ESamplerAddressMode addressMode);
	private:
		friend TSingleton<DefaultResources>;
		TStaticArray<RHITexturePtr, TEX_MAX_NUM> m_DefaultTexture2D;
		TStaticArray<RHITexturePtr, TEX_MAX_NUM> m_DefaultTextureCube;
		TStaticArray<TStaticArray<TUniquePtr<RHISampler>, (uint8)ESamplerFilter::MaxNum>, (uint8)ESamplerAddressMode::MaxNum> m_Samplers;
		DefaultResources();
		~DefaultResources() override;
		void CreateDefaultTextures();
		void CreateDefaultSamplers();
	};
}