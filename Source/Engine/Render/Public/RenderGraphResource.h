#pragma once
#include "Core/Public/String.h"
#include "RHI/Public/RHI.h"
namespace Render {
	enum class EResourceType {
		TEXTURE,
		BUFFER,
	};

	typedef uint32 ResourceID;

	class RGResource {
	private:
		friend class RenderGraph;
		friend class RGNodeBase;
		ResourceID m_ID;
		XXString m_Name;
		uint32 m_RefCount = 0;
	private:
		void AddRef();
	public:
		virtual ~RGResource() = 0; // must implement destruction in subclasses
		virtual EResourceType Type() = 0;
	};

	class RGBuffer: public RGResource {
	public:
		RGBuffer(const RHIBufferDesc& desc);
		explicit RGBuffer(RHIBufferDesc&& desc);
	private:
		RHIBufferDesc m_Desc;
		RHIBuffer* m_RHIBuffer{nullptr};
	};

	class RGTexture: public RGResource {
	public:
		struct Desc {
			
		};
	private:
	};
}