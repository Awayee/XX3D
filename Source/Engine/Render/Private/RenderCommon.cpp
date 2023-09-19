#include "Render/Public/RenderCommon.h"
#include "Core/Public/File.h"
#include "Core/Public/String.h"
#include "Core/Public/TSingleton.h"
#include "Render/Public/RenderReses.h"
#include "Resource/Public/Shaders.h"

#include "Asset/Public/AssetLoader.h"
#include "Asset/Public/TextureAsset.h"

namespace Engine {

	DescsMgr::DescsMgr() {
		GET_RHI(rhi);
		m_Layouts.Resize(DESCS_COUNT);
		// lighting, camera
		TVector<Engine::RSDescriptorSetLayoutBinding> sceneDescBindings;
		sceneDescBindings.PushBack({ Engine::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, Engine::SHADER_STAGE_FRAGMENT_BIT | Engine::SHADER_STAGE_VERTEX_BIT });
		sceneDescBindings.PushBack({ Engine::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, Engine::SHADER_STAGE_FRAGMENT_BIT | Engine::SHADER_STAGE_VERTEX_BIT });
		m_Layouts[DESCS_SCENE] = rhi->CreateDescriptorSetLayout(sceneDescBindings);
		// world transform
		TVector<Engine::RSDescriptorSetLayoutBinding> modelDescBindings;
		modelDescBindings.PushBack({ Engine::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, Engine::SHADER_STAGE_VERTEX_BIT });
		m_Layouts[DESCS_MODEL] = rhi->CreateDescriptorSetLayout(modelDescBindings);
		// material
		TVector<Engine::RSDescriptorSetLayoutBinding> materialDescBindings;
		materialDescBindings.PushBack({ Engine::DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, Engine::SHADER_STAGE_FRAGMENT_BIT });
		materialDescBindings.PushBack({ Engine::DESCRIPTOR_TYPE_SAMPLER, 1, Engine::SHADER_STAGE_FRAGMENT_BIT });
		m_Layouts[DESCS_MATERIAL] = rhi->CreateDescriptorSetLayout(materialDescBindings);
		// deferred lighting
		TVector<Engine::RSDescriptorSetLayoutBinding> deferredLightingBindings;
		deferredLightingBindings.PushBack({ Engine::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, Engine::SHADER_STAGE_FRAGMENT_BIT });// camera
		deferredLightingBindings.PushBack({ Engine::DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, Engine::SHADER_STAGE_FRAGMENT_BIT }); //light
		deferredLightingBindings.PushBack({ Engine::DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, Engine::SHADER_STAGE_FRAGMENT_BIT });//normal
		deferredLightingBindings.PushBack({ Engine::DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, Engine::SHADER_STAGE_FRAGMENT_BIT });//albedo
		deferredLightingBindings.PushBack({ Engine::DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, Engine::SHADER_STAGE_FRAGMENT_BIT });// depth
		m_Layouts[DESCS_DEFERRED_LIGHTING] = rhi->CreateDescriptorSetLayout(deferredLightingBindings);
	}

	DescsMgr::~DescsMgr()
	{
		GET_RHI(rhi);
		for(auto layout: m_Layouts) {
			rhi->DestroyDescriptorSetLayout(layout);
		}
		m_Layouts.Clear();
	}

	SamplerMgr::SamplerMgr()
	{
		GET_RHI(rhi);
		m_Samplers.Resize(SAMPLER_COUNT);
		Engine::RSSamplerInfo defaulIInfo{};
		defaulIInfo.minFilter = Engine::FILTER_LINEAR;
		defaulIInfo.magFilter = Engine::FILTER_LINEAR;
		defaulIInfo.addressModeU = Engine::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		defaulIInfo.addressModeV = Engine::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		defaulIInfo.addressModeW = Engine::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		m_Samplers[SAMPLER_DEFAULT] = rhi->CreateSampler(defaulIInfo);

		Engine::RSSamplerInfo deferredLightingInfo{};
		deferredLightingInfo.minFilter = Engine::FILTER_LINEAR;
		deferredLightingInfo.magFilter = Engine::FILTER_LINEAR;
		deferredLightingInfo.addressModeU = Engine::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		deferredLightingInfo.addressModeV = Engine::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		deferredLightingInfo.addressModeW = Engine::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		m_Samplers[SAMPLER_DEFERRED_LIGHTING] = rhi->CreateSampler(deferredLightingInfo);
	}

	SamplerMgr::~SamplerMgr()
	{
		GET_RHI(rhi);
		for(auto sampler: m_Samplers) {
			rhi->DestroySampler(sampler);
		}
		m_Samplers.Clear();
	}

	void TextureCommon::Create(Engine::RFormat format, uint32 width, uint32 height, Engine::RImageUsageFlags usage)
	{
		Release();
		GET_RHI(rhi);
		Image = rhi->CreateImage2D(format, width, height, 1, Engine::SAMPLE_COUNT_1_BIT, Engine::IMAGE_TILING_OPTIMAL, usage);
		Memory = rhi->CreateImageMemory(Image, Engine::MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
		const bool bForDepth = usage & Engine::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		View = rhi->CreateImageView(Image, Engine::IMAGE_VIEW_TYPE_2D, bForDepth ? Engine::IMAGE_ASPECT_DEPTH_BIT: Engine::IMAGE_ASPECT_COLOR_BIT,
			0, 1, 0, 1);
	}

	void TextureCommon::UpdatePixels(void* pixels, int channels) {
		BufferCommon b;
		b.CreateForTransfer(Image->GetExtent().w * Image->GetExtent().h * channels, pixels);
		RHI::Instance()->ImmediateCommit([&b, this](Engine::RCommandBuffer* cmd) {
			cmd->TransitionImageLayout(Image, Engine::IMAGE_LAYOUT_UNDEFINED, Engine::IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1, 0, 1, Engine::IMAGE_ASPECT_COLOR_BIT);
			cmd->CopyBufferToImage(b.Buffer, Image, Engine::IMAGE_ASPECT_COLOR_BIT, 0, 0, 1);
			cmd->TransitionImageLayout(Image, Engine::IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Engine::IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1, Engine::IMAGE_ASPECT_COLOR_BIT);
		});
	}

	void TextureCommon::Release()
	{
		GET_RHI(rhi);
		if(nullptr != Image) {
			rhi->DestroyImageView(View);
			View = nullptr;
			rhi->FreeMemory(Memory);
			Memory = nullptr;
			rhi->DestroyImage(Image);
			Image = nullptr;
		}
	}

	TextureMgr::TextureMgr(){
		m_DefaultTextures.Resize(MAX_NUM);
		m_DefaultTextures[WHITE].Create(FORMAT, DEFAULT_SIZE, DEFAULT_SIZE, USAGE);
		TVector<Math::UCVector4> whiteColors(DEFAULT_SIZE * DEFAULT_SIZE, { 255,255,255,255 });
		m_DefaultTextures[WHITE].UpdatePixels(whiteColors.Data(), CHANNELS);

		m_DefaultTextures[BLACK].Create(FORMAT, DEFAULT_SIZE, DEFAULT_SIZE, USAGE);
		TVector<Math::UCVector4> blackColors(DEFAULT_SIZE * DEFAULT_SIZE, { 0,0,0,255 });
		m_DefaultTextures[BLACK].UpdatePixels(blackColors.Data(), CHANNELS);

		m_DefaultTextures[GRAY].Create(FORMAT, DEFAULT_SIZE, DEFAULT_SIZE, USAGE);
		TVector<Math::UCVector4> grayColors(DEFAULT_SIZE * DEFAULT_SIZE, { 128,128,128,255 });
		m_DefaultTextures[GRAY].UpdatePixels(grayColors.Data(), CHANNELS);

		m_DefaultTextures[RED].Create(FORMAT, DEFAULT_SIZE, DEFAULT_SIZE, USAGE);
		TVector<Math::UCVector4> redColors(DEFAULT_SIZE * DEFAULT_SIZE, { 128,0,0,255 });
		m_DefaultTextures[RED].UpdatePixels(redColors.Data(), CHANNELS);

		m_DefaultTextures[GREEN].Create(FORMAT, DEFAULT_SIZE, DEFAULT_SIZE, USAGE);
		TVector<Math::UCVector4> greenColors(DEFAULT_SIZE * DEFAULT_SIZE, { 0,128,0,255 });
		m_DefaultTextures[GREEN].UpdatePixels(greenColors.Data(), CHANNELS);

		m_DefaultTextures[BLUE].Create(FORMAT, DEFAULT_SIZE, DEFAULT_SIZE, USAGE);
		TVector<Math::UCVector4> blueColors(DEFAULT_SIZE * DEFAULT_SIZE, { 0,0,128,255 });
		m_DefaultTextures[BLUE].UpdatePixels(blueColors.Data(), CHANNELS);
	}

	TextureCommon* TextureMgr::InstGetTexture(const char* file)
	{
		auto finded = m_TextureMap.find(file);
		if (finded != m_TextureMap.end()) {
			return &finded->second;
		}
		TextureCommon& tex = m_TextureMap.insert({ file, {} }).first->second;
		ATextureAsset imageAsset;
		if(!AssetLoader::LoadProjectAsset(&imageAsset, file)){
			return &m_DefaultTextures[GRAY];
		}
		tex.Create(FORMAT, imageAsset.Width, imageAsset.Height, USAGE);
		tex.UpdatePixels(imageAsset.Pixels.Data(), CHANNELS);
		return &tex;

	}

	void BufferCommon::Create(uint64 size, Engine::RBufferUsageFlags usage, Engine::RMemoryPropertyFlags memoryFlags, void* pData) {
		if (Size) Release();
		RHI::Instance()->CreateBufferWithMemory(size, usage, memoryFlags, Buffer, Memory, size, pData);
		Size = size;
		Usage = usage;
	}

	void BufferCommon::UpdateData(void* pData)
	{
		GET_RHI(rhi);
		void* p;
		rhi->MapMemory(Memory, &p);
		memcpy(p, pData, Size);
		rhi->UnmapMemory(Memory);
	}

	void BufferCommon::Release() {
		if(Size) {
			GET_RHI(rhi);
			rhi->DestroyBuffer(Buffer);
			Buffer = nullptr;
			rhi->FreeMemory(Memory);
			Memory = nullptr;
			Size = 0u;
			Usage = Engine::BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
		}
	}


	RenderPassCommon::~RenderPassCommon()
	{
		m_Attachments.Clear();
		GET_RHI(rhi);
		if(nullptr != m_Framebuffer) rhi->DestroyFramebuffer(m_Framebuffer);
		if(nullptr != m_RHIPass) rhi->DestroyRenderPass(m_RHIPass);
	}
	void RenderPassCommon::Begin(Engine::RCommandBuffer* cmd)
	{
		cmd->BeginRenderPass(m_RHIPass, m_Framebuffer, { 0, 0, m_Framebuffer->GetWidth(), m_Framebuffer->GetHeight() });
	}

	Engine::RImageView* RenderPassCommon::GetAttachment(uint32 attachmentIdx) const
	{
		ASSERT(attachmentIdx < m_Attachments.Size());
		return m_Attachments[attachmentIdx].View;
	}

	DeferredLightingPass::DeferredLightingPass()
	{
		GET_RHI(rhi);
		uint32 width = rhi->GetSwapchainExtent().w;
		uint32 height = rhi->GetSwapchainExtent().h;
		// create attachments
		m_Attachments.Resize(ATTACHMENT_COLOR_KHR);
		m_Attachments[ATTACHMENT_DEPTH].Create(rhi->GetDepthFormat(), width, height, Engine::IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|Engine::IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		m_Attachments[ATTACHMENT_NORMAL].Create(Engine::FORMAT_R8G8B8A8_UNORM, width, height, Engine::IMAGE_USAGE_COLOR_ATTACHMENT_BIT|Engine::IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		m_Attachments[ATTACHMENT_ALBEDO].Create(Engine::FORMAT_R8G8B8A8_UNORM, width, height, Engine::IMAGE_USAGE_COLOR_ATTACHMENT_BIT|Engine::IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		
		TVector<Engine::RSAttachment> attachments(ATTACHMENT_COUNT);
		// depth
		attachments[ATTACHMENT_DEPTH].Format = rhi->GetDepthFormat();
		attachments[ATTACHMENT_DEPTH].InitialLayout = Engine::IMAGE_LAYOUT_UNDEFINED;
		attachments[ATTACHMENT_DEPTH].FinalLayout = Engine::IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[ATTACHMENT_DEPTH].Clear = { 1.0, 0 };
		// g buffer normal
		attachments[ATTACHMENT_NORMAL].Format = Engine::FORMAT_R8G8B8A8_UNORM;
		attachments[ATTACHMENT_NORMAL].InitialLayout = Engine::IMAGE_LAYOUT_UNDEFINED;
		attachments[ATTACHMENT_NORMAL].FinalLayout = Engine::IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		attachments[ATTACHMENT_NORMAL].Clear = { 0.0f, 0.0f, 0.0f, 0.0f };
		// g buffer albedo
		attachments[ATTACHMENT_ALBEDO] = attachments[ATTACHMENT_NORMAL];
		// swapchain
		attachments[ATTACHMENT_COLOR_KHR].Format = rhi->GetSwapchainImageFormat();
		attachments[ATTACHMENT_COLOR_KHR].InitialLayout = Engine::IMAGE_LAYOUT_UNDEFINED;
		attachments[ATTACHMENT_COLOR_KHR].FinalLayout = Engine::IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[ATTACHMENT_COLOR_KHR].Clear = { 0.0f, 0.0f, 0.0f, 0.0f };

		// subpass
		m_ColorAttachments.Resize(SUBPASS_COUNT);
		m_ColorAttachments[SUBPASS_BASE] = { &m_Attachments[ATTACHMENT_NORMAL], &m_Attachments[ATTACHMENT_ALBEDO] };
		m_DepthAttachments.Resize(1);
		m_DepthAttachments[SUBPASS_BASE] = &m_Attachments[ATTACHMENT_DEPTH];
		TVector<Engine::RSubPassInfo> subpasses(SUBPASS_COUNT);
		// base pass
		subpasses[SUBPASS_BASE].ColorAttachments = {
			{ATTACHMENT_NORMAL, Engine::IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
			{ATTACHMENT_ALBEDO, Engine::IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
		};
		subpasses[SUBPASS_BASE].DepthStencilAttachment = { ATTACHMENT_DEPTH, Engine::IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		// lighting pass
		subpasses[SUBPASS_DEFERRED_LIGHTING].ColorAttachments = {
			{(uint32)attachments.Size() - 1, Engine::IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
		};
		subpasses[SUBPASS_DEFERRED_LIGHTING].InputAttachments = {
			{ATTACHMENT_NORMAL, Engine::IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{ATTACHMENT_ALBEDO, Engine::IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
			{ATTACHMENT_DEPTH,  Engine::IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
		};
		subpasses[SUBPASS_DEFERRED_LIGHTING].DepthStencilAttachment = { 0, Engine::IMAGE_LAYOUT_UNDEFINED };

		// dependencies
		TVector<Engine::RSubpassDependency> dependencies(2);
		dependencies[0].SrcSubPass = SUBPASS_INTERNAL;
		dependencies[0].DstSubPass = SUBPASS_BASE;
		dependencies[0].SrcStage = Engine::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | Engine::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].DstStage = Engine::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | Engine::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].SrcAccess = 0;
		dependencies[0].DstAccess = Engine::ACCESS_COLOR_ATTACHMENT_WRITE_BIT | Engine::ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		// lighting pass depends on base pass
		dependencies[1].SrcSubPass = SUBPASS_BASE;
		dependencies[1].DstSubPass = SUBPASS_DEFERRED_LIGHTING;
		dependencies[1].SrcStage = Engine::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | Engine::PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].DstStage = Engine::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | Engine::PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].SrcAccess = Engine::ACCESS_SHADER_WRITE_BIT | Engine::ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].DstAccess = Engine::ACCESS_SHADER_READ_BIT | Engine::ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependencies[1].DependencyFlags = Engine::DEPENDENCY_BY_REGION_BIT;

		m_RHIPass = rhi->CreateRenderPass(attachments, subpasses, dependencies);

		// create framebuffers
		uint32 maxImageCount = rhi->GetSwapchainMaxImageCount();
		m_SwapchainFramebuffers.Resize(maxImageCount);
		for(uint32 i=0; i<maxImageCount;++i) {
			TVector<Engine::RImageView*> attachments;
			for (auto& img : m_Attachments) {
				attachments.PushBack(img.View);
			}
			attachments.PushBack(rhi->GetSwapchainImageView(i));
			m_SwapchainFramebuffers[i] = rhi->CreateFrameBuffer(m_RHIPass, attachments, width, height, 1);
		}
		m_Framebuffer = m_SwapchainFramebuffers[0];
	}

	DeferredLightingPass::~DeferredLightingPass()
	{
		m_Framebuffer = nullptr;
		GET_RHI(rhi);
		for(auto framebuffer: m_SwapchainFramebuffers) {
			rhi->DestroyFramebuffer(framebuffer);
		}
		m_SwapchainFramebuffers.Clear();
	}

	void DeferredLightingPass::SetImageIndex(uint32 i)
	{
		m_Framebuffer = m_SwapchainFramebuffers[i];
	}

	GraphicsPipelineCommon::~GraphicsPipelineCommon()
	{
		GET_RHI(rhi);
		rhi->DestroyPipelineLayout(m_Layout);
		rhi->DestroyPipeline(m_Pipeline);
	}

	void GraphicsPipelineCommon::Bind(Engine::RCommandBuffer* cmd) {
		cmd->BindPipeline(m_Pipeline);
	}


	GBufferPipeline::GBufferPipeline(const RenderPassCommon* pass, uint32 subpass, const URect& area)
	{
		GET_RHI(rhi);
		// layout
		TVector<Engine::RDescriptorSetLayout*> setLayouts;
		setLayouts.PushBack(DescsMgr::Get(DESCS_SCENE));
		setLayouts.PushBack(DescsMgr::Get(DESCS_MODEL));
		setLayouts.PushBack(DescsMgr::Get(DESCS_MATERIAL));
		
		m_Layout = rhi->CreatePipelineLayout(setLayouts, {});
		// pipeline
		// shader
		TVector<int8> vertShaderCode;
		LoadShaderFile("GBufferMainVS.spv", vertShaderCode);
		TVector<int8> fragShaderCode;
		LoadShaderFile("GBufferMainPS.spv", fragShaderCode);
		Engine::RGraphicsPipelineCreateInfo info{};
		info.Shaders.PushBack({ Engine::SHADER_STAGE_VERTEX_BIT, vertShaderCode, "MainVS" });
		info.Shaders.PushBack({ Engine::SHADER_STAGE_FRAGMENT_BIT, fragShaderCode, "MainPS" });
		// input
		FillVertexInput(info.Bindings, info.Attributes);
		// viewport
		info.Viewport = { (float)area.x, (float)area.y, (float)area.w, (float)area.h, 0.0f, 1.0f };
		info.Scissor = area;
		// rasterization
		info.DepthClampEnable = false;
		info.RasterizerDiscardEnable = false;
		info.PolygonMode = Engine::POLYGON_MODE_FILL;
		info.CullMode = Engine::CULL_MODE_BACK_BIT;
		info.Clockwise = true;
		// depth
		info.DepthTestEnable = true;
		info.DepthWriteEnable = true;
		info.DepthCompareOp = Engine::COMPARE_OP_LESS;
		// blend
		info.LogicOpEnable = false;
		info.AttachmentStates.Resize(pass->GetColorAttachmentCount(subpass), {false});

		m_Pipeline = rhi->CreateGraphicsPipeline(info, m_Layout, pass->GetRHIPass(), subpass, nullptr, 0);
	}

	DeferredLightingPipeline::DeferredLightingPipeline(const RenderPassCommon* pass, uint32 subpass, const URect& area)
	{
		GET_RHI(rhi);
		TVector<Engine::RDescriptorSetLayout*> setLayouts{ DescsMgr::Get(DESCS_DEFERRED_LIGHTING) };
		m_Layout = rhi->CreatePipelineLayout(setLayouts, {});
		// shader
		TVector<int8> vertShaderCode;
		LoadShaderFile("DeferredLightingPBRMainVS.spv", vertShaderCode);
		TVector<int8> fragShaderCode;
		LoadShaderFile("DeferredLightingPBRMainPS.spv", fragShaderCode);
		Engine::RGraphicsPipelineCreateInfo info{};
		info.Shaders.PushBack({ Engine::SHADER_STAGE_VERTEX_BIT, vertShaderCode, "MainVS" });
		info.Shaders.PushBack({ Engine::SHADER_STAGE_FRAGMENT_BIT, fragShaderCode, "MainPS" });
		// no vertex input
		// viewport
		info.Viewport = {(float)area.x, (float)area.y, (float)area.w, (float)area.h, 0.0f, 1.0f};
		info.Scissor = area;
		// rasterization
		info.DepthClampEnable = false;
		info.RasterizerDiscardEnable = false;
		info.PolygonMode = Engine::POLYGON_MODE_FILL;
		info.CullMode = Engine::CULL_MODE_BACK_BIT;
		// depth
		info.DepthTestEnable = false;
		info.DepthWriteEnable = false;
		// blend
		info.LogicOpEnable = false;
		info.AttachmentStates.Resize(1, { false });//swapchain
		m_Pipeline = rhi->CreateGraphicsPipeline(info, m_Layout, pass->GetRHIPass(), subpass, nullptr, 0);
	}
}
