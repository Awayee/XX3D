#pragma once
#include "RHI/Public/RHI.h"
#include "Core/Public/BaseStructs.h"
#include "Core/Public/TSingleton.h"
#include "Core/Public/Container.h"
namespace Engine {
	enum EDescsType {
		DESCS_SCENE,
		DESCS_MODEL,
		DESCS_MATERIAL,
		DESCS_DEFERRED_LIGHTING,

		DESCS_COUNT
	};

	enum ESamplerType{
		SAMPLER_DEFAULT,
		SAMPLER_DEFERRED_LIGHTING,
		SAMPLER_COUNT
	};

	class DescsMgr: public TSingleton<DescsMgr> {

	private:
		TVector<Engine::RDescriptorSetLayout*> m_Layouts;
	public:
		DescsMgr();
		~DescsMgr();
		static Engine::RDescriptorSetLayout* Get(EDescsType type) { return Instance()->m_Layouts[type]; }
	};

	class SamplerMgr: public TSingleton<SamplerMgr> {
	private:
		TVector<Engine::RSampler*> m_Samplers;
	public:
		SamplerMgr();
		~SamplerMgr();
		static Engine::RSampler* Get(ESamplerType type) { return Instance()->m_Samplers[type]; }
	};

	struct TextureCommon {
		Engine::RImage* Image{nullptr};
		Engine::RImageView* View{nullptr};
		Engine::RMemory* Memory{nullptr};
		void Create(Engine::RFormat format, uint32 width, uint32 height, Engine::RImageUsageFlags usage);
		void UpdatePixels(void* pixels, int channels);
		void Release();
		~TextureCommon() { Release(); }
	};

	class TextureMgr : public TSingleton<TextureMgr> {
	private:
		const static Engine::RFormat FORMAT = Engine::FORMAT_R8G8B8A8_SRGB;
		const static Engine::RImageUsageFlags USAGE = Engine::IMAGE_USAGE_SAMPLED_BIT | Engine::IMAGE_USAGE_TRANSFER_DST_BIT;
		const static int CHANNELS = 4;
		const static uint32 DEFAULT_SIZE = 2;

		friend TSingleton<TextureMgr>;
		TStrMap<TextureCommon> m_TextureMap;
		TVector<TextureCommon> m_DefaultTextures;
		TextureMgr();
		~TextureMgr() {}
		TextureCommon* InstGetTexture(const char* file);
	public:
		enum DefaultType {
			WHITE,
			BLACK,
			GRAY,
			RED,
			BLUE,
			GREEN,
			MAX_NUM
		};
		static TextureCommon* Get(const char* file) { return Instance()->InstGetTexture(file); }
		static TextureCommon* Get(DefaultType type) { return &Instance()->m_DefaultTextures[type]; }
	};

	struct BufferCommon {
		Engine::RBuffer* Buffer{nullptr};
		Engine::RMemory* Memory{nullptr};
		uint64 Size{ 0u };
		Engine::RBufferUsageFlags Usage{Engine::BUFFER_USAGE_FLAG_BITS_MAX_ENUM};
	public:
		BufferCommon() = default;
		BufferCommon(const BufferCommon&) = default;
		BufferCommon(BufferCommon&&) = default;
		BufferCommon(uint64 size, Engine::RBufferUsageFlags usage, Engine::RMemoryPropertyFlags memoryFlags, void* pData) { Create(size, usage, memoryFlags, pData); }
		void Create(uint64 size, Engine::RBufferUsageFlags usage, Engine::RMemoryPropertyFlags memoryFlags, void* pData);
		void UpdateData(void* pData);
		void Release();
		~BufferCommon() { Release(); }

		// vertex buffer
		void CreateForVertex(uint64 size){
			Create(size, Engine::BUFFER_USAGE_VERTEX_BUFFER_BIT | Engine::BUFFER_USAGE_TRANSFER_DST_BIT, Engine::MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
		};
		// index buffer
		void CreateForIndex(uint64 size){
			Create(size, Engine::BUFFER_USAGE_INDEX_BUFFER_BIT | Engine::BUFFER_USAGE_TRANSFER_DST_BIT, Engine::MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
		};
		// staging buffer
		void CreateForTransfer(uint64 size, void* pData){
			Create(size, Engine::BUFFER_USAGE_TRANSFER_SRC_BIT, Engine::MEMORY_PROPERTY_HOST_COHERENT_BIT | Engine::MEMORY_PROPERTY_HOST_VISIBLE_BIT, pData);
		};
		// uniform buffer
		void CreateForUniform(uint64 size, void* pData){
			Create(size, Engine::BUFFER_USAGE_UNIFORM_BUFFER_BIT, Engine::MEMORY_PROPERTY_HOST_COHERENT_BIT | Engine::MEMORY_PROPERTY_HOST_VISIBLE_BIT, pData);
		};
		/*
		// vertex buffer
		static BufferCommon* CreateForVertex(uint64 size) {
			return new BufferCommon(size, RHI::BUFFER_USAGE_VERTEX_BUFFER_BIT | RHI::BUFFER_USAGE_TRANSFER_DST_BIT, RHI::MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
		}
		// index buffer
		static BufferCommon* CreateForIndex(uint64 size){
			return new BufferCommon(size, RHI::BUFFER_USAGE_INDEX_BUFFER_BIT | RHI::BUFFER_USAGE_TRANSFER_DST_BIT, RHI::MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
		}
		// staging buffer
		static BufferCommon* CreateForStaging(uint64 size, void* pData){
			return new BufferCommon(size, RHI::BUFFER_USAGE_TRANSFER_SRC_BIT, RHI::MEMORY_PROPERTY_HOST_COHERENT_BIT | RHI::MEMORY_PROPERTY_HOST_VISIBLE_BIT, pData);
		}
		// uniform buffer
		static BufferCommon* CreateForUniform(uint64 size){
			return new BufferCommon(size, RHI::BUFFER_USAGE_UNIFORM_BUFFER_BIT, RHI::MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
		}
		*/
	};

	class RenderPassCommon {
	protected:
		Engine::RRenderPass* m_RHIPass{nullptr};
		Engine::RFramebuffer* m_Framebuffer{nullptr};
		TVector<TextureCommon> m_Attachments;
		TVector<TVector<TextureCommon*>> m_ColorAttachments;
		TVector<TextureCommon*> m_DepthAttachments;
	public:
		virtual ~RenderPassCommon();
		Engine::RRenderPass* GetRHIPass() const { return m_RHIPass; }
		Engine::RFramebuffer* GetFramebuffer() const { return m_Framebuffer; }
		uint32 GetAttachmentCount() const { return m_Attachments.Size(); }
		Engine::RImageView* GetAttachment(uint32 attachmentIdx) const;
		uint32 GetColorAttachmentCount(uint32 subpass) const { ASSERT(subpass < m_ColorAttachments.Size()); return m_ColorAttachments[subpass].Size(); }
		Engine::RImageView* GetColorAttachment(uint32 subpass, uint32 idx) const { return m_ColorAttachments[subpass][idx]->View; }
		Engine::RImageView* GetDepthAttachment(uint32 subpass) const { return m_DepthAttachments[subpass]->View; }
		virtual void Begin(Engine::RCommandBuffer* cmd);
	};

	class DeferredLightingPass final: public RenderPassCommon {
	private:
		TVector<Engine::RFramebuffer*> m_SwapchainFramebuffers;
	public:
		enum {
			ATTACHMENT_DEPTH,
			ATTACHMENT_NORMAL,
			ATTACHMENT_ALBEDO,
			ATTACHMENT_COLOR_KHR,
			ATTACHMENT_COUNT,
		};

		enum {
			SUBPASS_BASE,
			SUBPASS_DEFERRED_LIGHTING,
			SUBPASS_COUNT
		};
		DeferredLightingPass();
		~DeferredLightingPass();
		void SetImageIndex(uint32 i);
	};

	class GraphicsPipelineCommon {
	protected:
		Engine::RPipelineLayout* m_Layout;
		Engine::RPipeline* m_Pipeline;
	public:
		virtual ~GraphicsPipelineCommon();
		Engine::RPipelineLayout* GetLayout()const { return m_Layout; }
		void Bind(Engine::RCommandBuffer* cmd);
	};

	class GBufferPipeline: public GraphicsPipelineCommon {
	public:
		GBufferPipeline(const RenderPassCommon* pass, uint32 subpass, const URect& area);
	};

	class DeferredLightingPipeline: public GraphicsPipelineCommon {
	public:
		DeferredLightingPipeline(const RenderPassCommon* pass, uint32 subpass, const URect& area);
	};
}