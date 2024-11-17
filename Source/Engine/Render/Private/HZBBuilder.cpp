#include "Render/Public/HZBBuilder.h"
#include "Render/Public/GlobalShader.h"
#include "Render/Public/DefaultResource.h"

namespace {
	class HZBCS : public Render::GlobalShader {
		GLOBAL_SHADER_IMPLEMENT(HZBCS, "HZB.hlsl", "MainCS", EShaderStageFlags::Compute);
		SHADER_PERMUTATION_BEGIN_SWITCH(CLOSEST, false);
		SHADER_PERMUTATION_NEXT_SWITCH(FURTHEST, true, CLOSEST);
		SHADER_PERMUTATION_NEXT_SWITCH(OUTPUT2X2, true, FURTHEST);
		SHADER_PERMUTATION_END(OUTPUT2X2);
	};

	class TextureBlitCS : public Render::GlobalShader {
		GLOBAL_SHADER_IMPLEMENT(TextureBlitCS, "TextureBlit.hlsl", "MainCS", EShaderStageFlags::Compute);
		SHADER_PERMUTATION_EMPTY();
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
				cmd->SetShaderParam(0, 0, RHIShaderParam::Texture(depth, RHITextureSubRes{ 0, 1, 0, 1, ETextureDimension::Tex2D, ETextureViewFlags::Depth }));
				cmd->SetShaderParam(0, 1, RHIShaderParam::TextureUAV(m_HZB.Get(), dstSubRes));
				cmd->SetShaderParam(0, 2, RHIShaderParam::UniformBuffer(sizeBuffer));
				RHISampler* sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Point, ESamplerAddressMode::Clamp);
				cmd->SetShaderParam(0, 3, RHIShaderParam::Sampler(sampler));
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
				cmd->SetShaderParam(0, 0, RHIShaderParam::Texture(m_HZB.Get(), srcSubRes));
				cmd->SetShaderParam(0, 1, RHIShaderParam::UniformBuffer(sizeBuffer));
				cmd->SetShaderParam(0, 2, RHIShaderParam::TextureUAV(m_HZB.Get(), dstSubRes));
				cmd->Dispatch(groupX, groupY, 1);
			});
		}

		return hzbNode;
	}

	RHIComputePipelineState* HZBBuilder::GetHZBPSO(HZBPSOFlags flags) {
		const uint32 flagVal = flags.Val;
		CHECK(flagVal < m_HZBPSOStorage.Size());
		if(!m_HZBPSOStorage[flagVal]) {
			RHIComputePipelineStateDesc desc;
			desc.Layout.Resize(1);
			auto& layout = desc.Layout[0];
			layout.Reserve(4);
			layout.PushBack({ EBindingType::Texture, EShaderStageFlags::Compute, 1 });
			layout.PushBack({ EBindingType::UniformBuffer, EShaderStageFlags::Compute, 1 });
			if(flags.Furthest) {
				layout.PushBack({ EBindingType::StorageTexture, EShaderStageFlags::Compute, 1 });
			}
			if(flags.Closest) {
				layout.PushBack({ EBindingType::StorageTexture, EShaderStageFlags::Compute, 1 });
			}
			HZBCS::ShaderPermutation csp;
			csp.FURTHEST = flags.Furthest;
			csp.CLOSEST = flags.Closest;
			csp.OUTPUT2X2 = flags.Output2x2;
			RHIShader* cs = GlobalShaderMap::Instance()->GetShader<HZBCS>(csp)->GetRHI();
			desc.Shader = cs;
			m_HZBPSOStorage[flagVal] = RHI::Instance()->CreateComputePipelineState(desc);
		}
		return m_HZBPSOStorage[flagVal].Get();
	}

	void HZBBuilder::CreateBlitPSO() {
		RHIComputePipelineStateDesc desc;
		desc.Layout = {{
			{EBindingType::Texture, EShaderStageFlags::Compute, 1},
			{EBindingType::StorageTexture, EShaderStageFlags::Compute, 1},
			{EBindingType::UniformBuffer, EShaderStageFlags::Compute, 1},
			{EBindingType::Sampler, EShaderStageFlags::Compute, 1}
		}};
		RHIShader* cs = GlobalShaderMap::Instance()->GetShader<TextureBlitCS>()->GetRHI();
		desc.Shader = cs;
		m_BlitPSO = RHI::Instance()->CreateComputePipelineState(desc);
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
