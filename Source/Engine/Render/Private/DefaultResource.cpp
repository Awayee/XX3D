#include "Render/Public/DefaultResource.h"
#include "Core/Public/Func.h"

namespace Render {

	RHITexture* DefaultResources::GetDefaultTexture2D(DefaultTextureType type) {
		return m_DefaultTexture2D[type].Get();
	}

	RHITexture* DefaultResources::GetDefaultTextureCube(DefaultTextureType type) {
		return m_DefaultTextureCube[type].Get();
	}

	RHISampler* DefaultResources::GetDefaultSampler(ESamplerFilter filter, ESamplerAddressMode addressMode) {
		return m_Samplers[(uint8)addressMode][(uint8)filter].Get();
	}

	DefaultResources::DefaultResources() {
		CreateDefaultTextures();
		CreateDefaultSamplers();
	}

	DefaultResources::~DefaultResources() {	
	}

	void DefaultResources::CreateDefaultTextures() {
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
		desc.MipSize = 1;
		desc.Samples = 1;

		for (uint32 i = 0; i < DefaultTextureType::TEX_MAX_NUM; ++i) {
			m_DefaultTexture2D[i] = rhi->CreateTexture(desc);
		}
		desc.Dimension = ETextureDimension::TexCube;
		desc.ArraySize = 6;
		for (uint32 i = 0; i < DefaultTextureType::TEX_MAX_NUM; ++i) {
			m_DefaultTextureCube[i] = rhi->CreateTexture(desc);
		}


		// white;
		auto* tex = GetDefaultTexture2D(TEX_WHITE);
		const Color4<uint8> color{ 1,1,1,1 };
		const RHITextureOffset offset{ 0,0, {0,0,0} };
		tex->UpdateData(offset, sizeof(color), &color);
	}

	void DefaultResources::CreateDefaultSamplers() {
		RHISamplerDesc desc;
		for (uint8 addressMode = 0; addressMode < (uint8)ESamplerAddressMode::MaxNum; ++addressMode) {
			desc.AddressU = desc.AddressV = desc.AddressW = (ESamplerAddressMode)addressMode;
			for (uint8 filter = 0; filter < (uint8)ESamplerFilter::MaxNum; ++filter) {
				desc.Filter = (ESamplerFilter)filter;
				m_Samplers[addressMode][filter] = RHI::Instance()->CreateSampler(desc);
			}
		}
	}

}
