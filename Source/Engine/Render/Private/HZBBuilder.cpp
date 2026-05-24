#include "Render/Public/HZBBuilder.h"
#include "Render/Public/GlobalShader.h"
#include "Render/Public/DefaultResource.h"

namespace {
	class HZBCS : public Render::GlobalShader {
		BEGIN_SHADER_PERMUTATION
		SHADER_PERMUTATION_SWITCH(CLOSEST, false)
		SHADER_PERMUTATION_SWITCH(FURTHEST, true)
		SHADER_PERMUTATION_SWITCH(OUTPUT2X2, true)
		END_SHADER_PERMUTATION
		BEGIN_SHADER_BINDING
		SHADER_BINDING(0, Texture, inDepth, 1)
		SHADER_BINDING(1, UniformBuffer, uSizeInfo,1)
		SHADER_BINDING_WITH_MACRO(2, RWTexture, outFurthest, 1, FURTHEST, true)
		SHADER_BINDING_WITH_MACRO(3, RWTexture, outClosest, 1, CLOSEST, true)
		END_SHADER_BINDING
		GLOBAL_SHADER_IMPLEMENT(HZBCS, "HZB.hlsl", "MainCS", EShaderStageFlags::Compute);
	};

	class TextureBlitCS : public Render::GlobalShader {
		SHADER_PERMUTATION_EMPTY();
		BEGIN_SHADER_BINDING
		SHADER_BINDING(0, Texture, srcTex, 1)
		SHADER_BINDING(1, RWTexture, dstTex, 1)
		SHADER_BINDING(2, UniformBuffer, uSizeInfo, 1)
		SHADER_BINDING(3, Sampler, inSampler, 1)
		END_SHADER_BINDING
		GLOBAL_SHADER_IMPLEMENT(TextureBlitCS, "TextureBlit.hlsl", "MainCS", EShaderStageFlags::Compute);
	};

	static constexpr uint32 THREAD_ROW_NUM = 16; // num of threads per thread group
	static constexpr uint32 PER_GROUP_OUTPUT_ROW = THREAD_ROW_NUM * 2; // num of output pixels per thread group
	static constexpr uint32 MIN_DEPTH_SIZE = 1;
}

namespace Render {

	inline uint32 GetHZBSize(uint32 srcWidth, uint32 srcHeight) {
		const uint32 targetSize = Math::Max(srcWidth, srcHeight);
		return Math::LowerPowerOf2(targetSize);
	}

	HZBBuilder::HZBBuilder() {
		CreateBlitPSO();
	}

	void HZBBuilder::Update(uint32 srcWidth, uint32 srcHeight, const Math::FMatrix4x4& generatedByMatrix) {
		m_ViewProjectMatrix = generatedByMatrix;
		if(srcWidth ==0 || srcHeight == 0) {
			return;
		}
		const uint32 dstSize = GetHZBSize(srcWidth, srcHeight);
		if (dstSize <= MIN_DEPTH_SIZE) {
			return;
		}
		// recreate texture
		if (!m_HZB || dstSize != m_HZB->GetDesc().Width) {
			CreateOutputTexture(dstSize);
		}
		// calculate hzb actual size
		const float srcAspect = (float)srcWidth / (float)srcHeight;
		const float hzbAspect = (float)m_ActualWidth / (float)m_ActualHeight;
		if(!Math::FloatEqual(hzbAspect, srcAspect)) {
			// aspect ratio < 1.0f
			if (srcWidth < srcHeight) {
				m_ActualHeight = dstSize;
				m_ActualWidth = (uint32)Math::Ceil((float)m_ActualHeight * srcAspect);
			}
			else {
				m_ActualWidth = dstSize;
				m_ActualHeight = (uint32)Math::Ceil((float)m_ActualWidth / srcAspect);
			}
		}
	}

	RGTextureNode* HZBBuilder::BuildFurthest(RHITexture* depth, Render::RGTextureNode* depthNode, Render::RenderGraph& rg) {
		if(!m_HZB) {
			return nullptr;
		}
		// calculate target texture size
		const auto& srcDesc = depth->GetDesc();
		const uint32 srcWidth = srcDesc.Width;
		const uint32 srcHeight = srcDesc.Height;
		uint32 dstSize = m_HZB->GetDesc().Width;
		// get pso
		HZBPSOFlags psoFlags;
		psoFlags.Closest = false;
		psoFlags.Furthest = true;
		psoFlags.Output2x2 = true;

		// config render graph
		RGTextureNode* hzbNode = rg.CreateTextureNode(m_HZB, "HZB");
		// generate mip 0 firstly
		{
			RGComputeNode* buildHZBMip0 = rg.CreateComputeNode("BuildHZBMip0");
			const RHITextureSubRes srcSubRes = RHITextureSubRes{ 0, 1, 0, 1, ETextureDimension::Tex2D, ETextureViewFlags::DepthStencil };
			buildHZBMip0->ReadSRV(depthNode, srcSubRes);
			const RHITextureSubRes dstSubRes = RHITextureSubRes{ 0, 1, 0, 1, ETextureDimension::Tex2D, ETextureViewFlags::Color };
			buildHZBMip0->WriteUAV(hzbNode, dstSubRes);

			// calculate thread group size
			uint32 groupX = Math::Max(1u, (m_ActualWidth + PER_GROUP_OUTPUT_ROW - 1) / PER_GROUP_OUTPUT_ROW);
			uint32 groupY = Math::Max(1u, (m_ActualHeight + PER_GROUP_OUTPUT_ROW - 1) / PER_GROUP_OUTPUT_ROW);

			// size buffer
			Math::FVector4 sizeInfo = { (float)m_ActualWidth, (float)m_ActualHeight, (float)srcWidth, (float)srcHeight};
			RHIDynamicBuffer sizeBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(sizeInfo), &sizeInfo, 0);

			buildHZBMip0->SetTask([this, depth, dstSubRes, sizeBuffer, groupX, groupY](RHICommandBuffer* cmd) {
				cmd->BindComputePipeline(m_BlitPSO.Get());
				cmd->SetShaderParam(TextureBlitCS::srcTex, RHIShaderParam::Texture(depth, RHITextureSubRes{ 0, 1, 0, 1, ETextureDimension::Tex2D, ETextureViewFlags::Depth }));
				cmd->SetShaderParam(TextureBlitCS::dstTex, RHIShaderParam::TextureUAV(m_HZB.Get(), dstSubRes));
				cmd->SetShaderParam(TextureBlitCS::uSizeInfo, RHIShaderParam::UniformBuffer(sizeBuffer));
				RHISampler* sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Point, ESamplerAddressMode::Clamp);
				cmd->SetShaderParam(TextureBlitCS::inSampler, RHIShaderParam::Sampler(sampler));
				cmd->Dispatch(groupX, groupY, 1);
			});
		}

		RHIComputePipelineState* pso = GetHZBPSO(psoFlags);
		// generate other mips
		const uint8 mipSize = m_HZB->GetDesc().MipSize;
		for(uint8 i=1; i<mipSize; ++i) {
			dstSize >>= 1;
			RGTextureNode* lastHzbNode = hzbNode;
			hzbNode = rg.CopyTextureNode(hzbNode, StringFormat("HZB%u", i));
			RGComputeNode* buildHZBMip = rg.CreateComputeNode(StringFormat("BuildHZBMip%u", i));
			const RHITextureSubRes srcSubRes = RHITextureSubRes{ 0, 1, (uint8)(i-1), 1, ETextureDimension::Tex2D, ETextureViewFlags::Color };
			buildHZBMip->ReadSRV(lastHzbNode, srcSubRes);
			const RHITextureSubRes dstSubRes = RHITextureSubRes{ 0, 1, i, 1, ETextureDimension::Tex2D, ETextureViewFlags::Color };
			buildHZBMip->WriteUAV(hzbNode, dstSubRes);
			// calculate thread group num
			float widthScale = (float)dstSize / (float)srcWidth;
			uint32 heightSize = (uint32)Math::Ceil(widthScale * (float)srcHeight);
			uint32 groupX = Math::Max(1u, (dstSize + PER_GROUP_OUTPUT_ROW - 1) / PER_GROUP_OUTPUT_ROW);
			uint32 groupY = Math::Max(1u, (heightSize + PER_GROUP_OUTPUT_ROW - 1) / PER_GROUP_OUTPUT_ROW);
			Math::IVector4 sizeInfo = { (int)dstSize };

			RHIDynamicBuffer sizeBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(sizeInfo), &sizeInfo, 0);

			buildHZBMip->SetTask([this, pso, srcSubRes, dstSubRes, groupX, groupY, sizeBuffer](RHICommandBuffer* cmd) {
				cmd->BindComputePipeline(pso);
				cmd->SetShaderParam(HZBCS::inDepth, RHIShaderParam::Texture(m_HZB.Get(), srcSubRes));
				cmd->SetShaderParam(HZBCS::uSizeInfo, RHIShaderParam::UniformBuffer(sizeBuffer));
				cmd->SetShaderParam(HZBCS::outFurthest, RHIShaderParam::TextureUAV(m_HZB.Get(), dstSubRes));
				cmd->Dispatch(groupX, groupY, 1);
			});
		}

		return hzbNode;
	}

	RHIComputePipelineState* HZBBuilder::GetHZBPSO(HZBPSOFlags flags) {
		const uint32 flagVal = flags.Val;
		CHECK(flagVal < m_HZBPSOStorage.Size());
		if(!m_HZBPSOStorage[flagVal]) {
			HZBCS::ShaderPermutation csp;
			csp.FURTHEST = flags.Furthest;
			csp.CLOSEST = flags.Closest;
			csp.OUTPUT2X2 = flags.Output2x2;
			RHIShader* cs = GlobalShaderMap::Instance()->GetShader<HZBCS>(csp)->GetRHI();
			m_HZBPSOStorage[flagVal] = RHI::Instance()->CreateComputePipelineState(cs);
		}
		return m_HZBPSOStorage[flagVal].Get();
	}

	void HZBBuilder::CreateBlitPSO() {
		RHIShader* cs = GlobalShaderMap::Instance()->GetShader<TextureBlitCS>()->GetRHI();
		m_BlitPSO = RHI::Instance()->CreateComputePipelineState(cs);
	}

	void HZBBuilder::CreateOutputTexture(uint32 dstSize) {
		RHITextureDesc desc = RHITextureDesc::Texture2D();
		desc.Flags = ETextureFlags::SRV | ETextureFlags::UAV;
		desc.Width = desc.Height = dstSize;
		desc.Format = ERHIFormat::R32_SFLOAT;
		// calc mip size, min tex size is (MIN_DEPTH_SIZE x MIN_DEPTH_SIZE)
		uint8 mipSize = 0;
		while (dstSize >= MIN_DEPTH_SIZE) {
			++mipSize;
			dstSize >>= 1;
		}
		desc.MipSize = mipSize;
		desc.ArraySize = 1;
		m_HZB = RHI::Instance()->CreateTexture(desc);
		m_HZB->SetName("HZB");
	}
}
