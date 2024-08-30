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
		RHITextureDesc desc = RHITextureDesc::Texture2D();
		desc.Format = ERHIFormat::R8G8B8A8_UNORM;
		desc.Flags = TEXTURE_FLAG_SRV | TEXTURE_FLAG_CPY_DST;
		desc.Width = desc.Height = 1;

		for (uint32 i = 0; i < DefaultTextureType::TEX_MAX_NUM; ++i) {
			m_DefaultTexture2D[i] = rhi->CreateTexture(desc);
		}
		desc.Dimension = ETextureDimension::TexCube;
		desc.ArraySize = 6;
		for (uint32 i = 0; i < DefaultTextureType::TEX_MAX_NUM; ++i) {
			m_DefaultTextureCube[i] = rhi->CreateTexture(desc);
		}

		// white
		{
			auto* tex = GetDefaultTexture2D(TEX_WHITE);
			const U8Color4 color{ 255,255,255,255 };
			const RHITextureOffset offset{ 0,0, {0,0,0} };
			tex->UpdateData(sizeof(color), &color, offset);
			auto* texCube = GetDefaultTextureCube(TEX_WHITE);
			TStaticArray<U8Color4, 6> colors;
			for(auto& c: colors) {
				c = color;
			}
			texCube->UpdateData(colors.ByteSize(), colors.Data(), offset);
		}
		// black
		{
			auto* tex = GetDefaultTexture2D(TEX_BLACK);
			const U8Color4 color{ 0,0,0,255 };
			const RHITextureOffset offset{ 0,0, {0,0,0} };
			tex->UpdateData(sizeof(color), &color, offset);
			auto* texCube = GetDefaultTextureCube(TEX_BLACK);
			TStaticArray<U8Color4, 6> colors;
			for (auto& c : colors) {
				c = color;
			}
			texCube->UpdateData(colors.ByteSize(), colors.Data(), offset);
		}
		// gray
		{
			auto* tex = GetDefaultTexture2D(TEX_GRAY);
			const U8Color4 color{ 127,127,127,255 };
			const RHITextureOffset offset{ 0,0, {0,0,0} };
			tex->UpdateData(sizeof(color), &color, offset);
			auto* texCube = GetDefaultTextureCube(TEX_GRAY);
			TStaticArray<U8Color4, 6> colors;
			for (auto& c : colors) {
				c = color;
			}
			texCube->UpdateData(colors.ByteSize(), colors.Data(), offset);
		}
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

	RHISampler* GetDefaultSampler(ESamplerFilter filter, ESamplerAddressMode addressMode) {
		return DefaultResources::Instance()->GetDefaultSampler(filter, addressMode);
	}

}
