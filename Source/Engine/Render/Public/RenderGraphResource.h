#pragma once
#include "Core/Public/String.h"
#include "RHI/Public/RHI.h"
namespace Engine {
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
		struct Desc {
			uint64 Size;
			RBufferUsageFlags Usage;
			RMemoryPropertyFlags MemoryFlags;
			void* pData = nullptr;
		};
		RGBuffer(const Desc& desc);
		explicit RGBuffer(Desc&& desc);
	private:
		Desc m_Desc;
		RBuffer* m_RHIBuffer{nullptr};
	};

	class RGTexture: public RGResource {
	public:
		struct Desc {
			
		};
	private:
	};
}