#include "Render/Public/Textures.h"
#include "Core/Public/Func.h"

namespace Render {

	RHITexture* DefaultTexture::GetDefaultTexture2D(DefaultTextureType type) {
		return m_DefaultTexture2D[type].Get();
	}

	RHITexture* DefaultTexture::GetDefaultTextureCube(DefaultTextureType type) {
		return m_DefaultTextureCube[type].Get();
	}

	DefaultTexture::DefaultTexture() {
		// create default texture 2d;
		RHI* rhi = RHI::Instance();
		ASSERT(rhi, "");
		RHITextureDesc desc;
		desc.Dimension = ETextureDimension::Tex2D;
		desc.Format = ERHIFormat::R8G8B8A8_UNORM;
		desc.Flags = TEXTURE_FLAG_SRV | TEXTURE_FLAG_CPY_DST;
		desc.Size = { 1,1,1 };
		desc.Depth = 1;
		desc.ArraySize = 1;
		desc.NumMips = 1;
		desc.Samples = 1;

		for (uint32 i = 0; i < DefaultTextureType::MAX_NUM; ++i) {
			m_DefaultTexture2D[i] = rhi->CreateTexture(desc, nullptr);
		}
		desc.Dimension = ETextureDimension::TexCube;
		desc.ArraySize = 6;
		for(uint32 i=0; i<DefaultTextureType::MAX_NUM; ++i) {
			m_DefaultTextureCube[i] = rhi->CreateTexture(desc, nullptr);
		}


		// white;
		auto* tex = GetDefaultTexture2D(WHITE);
		const Color4<uint8> color{ 1,1,1,1 };
		const RHITextureOffset offset{ 0,0, {0,0,0} };
		tex->UpdateData(offset, sizeof(color), &color);
	}

	DefaultTexture::~DefaultTexture() {	
	}

}
