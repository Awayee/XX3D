#pragma once
#include "Core/Public/TSingleton.h"
#include "Core/Public/TArray.h"
#include "Core/Public/TUniquePtr.h"
#include "RHI/Public/RHI.h"

namespace Engine {
	class DefaultTexture: public TSingleton<DefaultTexture> {
	public:
		enum DefaultTextureType : uint8 {
			WHITE=0,
			BLACK,
			GRAY,
			MAX_NUM
		};
		RHITexture* GetTexture(DefaultTextureType type, ETextureDimension textureDimension);
	private:
		friend TSingleton<DefaultTexture>;
		DefaultTexture();
		~DefaultTexture() override;

		TStaticArray<TStaticArray<TUniquePtr<RHITexture>, MAX_NUM>, (uint8)ETextureDimension::MaxNum> m_DefaultTextures;
	};
}