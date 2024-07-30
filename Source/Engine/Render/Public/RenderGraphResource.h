#pragma once
#include "Core/Public/String.h"
#include "RHI/Public/RHI.h"
namespace Render {
	enum class EResourceType {
		TEXTURE,
		BUFFER,
	};

	typedef uint32 ResourceID;

	class RGResourceNode {
	public:
		virtual ~RGResourceNode() = 0; // must implement destruction in subclasses
		virtual EResourceType Type() = 0;
	private:
		friend class RenderGraph;
		friend class RGPassNode;
		ResourceID m_ID;
		XXString m_Name;
		uint32 m_RefCount = 0;
		void AddRef();
	};

	class RGBufferNode: public RGResourceNode {
	public:
		NON_COPYABLE(RGBufferNode);
		RGBufferNode(const RHIBufferDesc& desc);
	private:
		RHIBufferDesc m_Desc;
		RHIBuffer* m_RHIBuffer{nullptr};
	};

	class RGTextureNode: public RGResourceNode {
	public:
		NON_COPYABLE(RGTextureNode);
		RGTextureNode(const RHITextureDesc& desc);
	private:
		RHITextureDesc m_Desc;
		RHITexture* m_RHITexture{ nullptr };
	};
}