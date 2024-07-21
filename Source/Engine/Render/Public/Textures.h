#pragma once
#include "Core/Public/TSingleton.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHI.h"

namespace Render {
	class DefaultTexture: public TSingleton<DefaultTexture> {
	public:
		enum DefaultTextureType: uint32 {
			WHITE=0,
			BLACK,
			GRAY,
			MAX_NUM
		};
		RHITexture* GetDefaultTexture2D(DefaultTextureType type);
		RHITexture* GetDefaultTextureCube(DefaultTextureType type);
		RHITexture* GetTexture(DefaultTextureType type, ETextureDimension textureDimension);
	private:
		friend TSingleton<DefaultTexture>;
		DefaultTexture();
		~DefaultTexture() override;
		TStaticArray<RHITexturePtr, MAX_NUM> m_DefaultTexture2D;
		TStaticArray<RHITexturePtr, MAX_NUM> m_DefaultTextureCube;
	};
}