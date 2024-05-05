#include "Render/Public/Textures.h"
#include "Core/Public/Func.h"

namespace Engine {

	RHITexture* DefaultTexture::GetTexture(DefaultTextureType type, ETextureDimension textureDimension) {
		return m_DefaultTextures[(uint8)textureDimension][type].Get(); 
	}

	DefaultTexture::DefaultTexture() {
		// create default textures;
		RHI* rhi = RHI::Instance();
		ASSERT(rhi, "");
		RHITextureDesc desc;
		desc.Dimension = ETextureDimension::Tex2D;
		desc.Format = ERHIFormat::R8G8B8A8_UNORM;
		desc.Flags = TEXTURE_FLAG_SRV | TEXTURE_FLAG_CPY_SRC;
		desc.Size = { 1,1,1 };
		desc.Depth = 1;
		desc.ArraySize = 1;
		desc.NumMips = 1;
		desc.Samples = 1;

		for(uint8 i=0; i<(uint8)ETextureDimension::MaxNum; ++i) {
			for(uint8 j=0; j<(uint8)DefaultTextureType::MAX_NUM; ++j) {
				m_DefaultTextures[i][j] = rhi->CreateTexture(desc, nullptr);
			}
		}

		// white;
		auto* tex = GetTexture(WHITE, ETextureDimension::Tex2D);
		const Color4<uint16> color{ 1,1,1,1 };
		const RHITextureOffset offset{ 0,0,1,{0,0,0} };
		const USize3D size = { 1,1,1 };
		tex->UpdateData(offset, size, &color);
	}

	DefaultTexture::~DefaultTexture() {	
	}

}
