#pragma once
#include "Core/Public/Defines.h"
#include "Core/Public/COntainer.h"
#include "Core/Public/String.h"
#include "Asset/Public/TextureAsset.h"
#include "RHI/Public/RHI.h"

namespace Object {
	class TextureResource {
	public:
		NON_COPYABLE(TextureResource);
		TextureResource(const Asset::TextureAsset& asset);
		~TextureResource() = default;
		TextureResource(TextureResource&& rhs) noexcept;
		TextureResource& operator=(TextureResource&& rhs) noexcept;
		RHITexture* GetRHI();
		static ERHIFormat ConvertTextureFormat(Asset::ETextureAssetType assetType);
	private:
		RHITexturePtr m_RHI;
	};

	class TextureResourceMgr {
		SINGLETON_INSTANCE(TextureResourceMgr);
	public:
		RHITexture* GetTexture(const XString& fileName);
	private:
		TMap<XString, TextureResource> m_All;
		TextureResourceMgr() = default;
		~TextureResourceMgr() = default;
	};

	// pre-initialized pipeline states
	class StaticPipelineStateMgr {
		SINGLETON_INSTANCE(StaticPipelineStateMgr);
	public:
		typedef void(*ComputePipelineInitializer)(RHIComputePipelineStateDesc&);
		typedef void(*GraphicsPipelineInitializer)(RHIGraphicsPipelineStateDesc& desc);
		static uint32 RegisterPSOInitializer(GraphicsPipelineInitializer func);
		static uint32 RegisterPSOInitializer(ComputePipelineInitializer func);
		void ReCreatePipelineState(uint32 psoID, const RHIGraphicsPipelineStateDesc& desc);
		RHIGraphicsPipelineState* GetGraphicsPipelineState(uint32 psoID);
		RHIComputePipelineState* GetComputePipelineState(uint32 psoID);
	private:
		TArray<RHIGraphicsPipelineStatePtr> m_GraphicsPipelineStates;
		TArray<RHIComputePipelineStatePtr> m_ComputePipelineStates;
		StaticPipelineStateMgr();
		~StaticPipelineStateMgr() = default;
	};
}