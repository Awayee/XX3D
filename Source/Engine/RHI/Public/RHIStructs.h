#pragma once
#include "RHIEnum.h"
#include "Core/Public/TVector.h"
#include "Core/Public/TypeDefine.h"
#include "Core/Public/BaseStructs.h"

namespace Engine {
	struct RSInitInfo {
		bool EnableDebug;
		bool EnableGeometryShader;
		USize2D WindowSize;
		void* WindowHandle;
		const char* ApplicationName;
		uint8 MaxFramesInFlight;
	};

	struct RSViewport {
		float x;
		float y;
		float width;
		float height;
		float minDepth;
		float maxDepth;
	};

	struct RSClear {
		RClearValueType Type{CLEAR_VALUE_NONE};
		union {
			struct {
				float r;
				float g;
				float b;
				float a;
			}Color;
			struct {
				float depth;
				uint32 stencil;
			}DepthStencil;
		};
		RSClear() = default;
		RSClear(float r, float g, float b, float a) {
			Type = CLEAR_VALUE_COLOR;
			Color.r = r; Color.g = g; Color.b = b; Color.a = a;
		}
		RSClear(float depth, uint32 stencil) {
			Type = CLEAR_VALUE_DEPTH_STENCIL;
			DepthStencil.depth = depth; DepthStencil.stencil = stencil;
		}
	};

	struct RSAttachment {
		RFormat Format;
		RImageLayout InitialLayout{ IMAGE_LAYOUT_UNDEFINED };
		RImageLayout FinalLayout{ IMAGE_LAYOUT_UNDEFINED };
		RSampleCountFlagBits SampleCount{ SAMPLE_COUNT_1_BIT };
		RAttachmentLoadOp LoadOp{ ATTACHMENT_LOAD_OP_CLEAR };
		RAttachmentStoreOp StoreOp{ ATTACHMENT_STORE_OP_STORE };
		RAttachmentLoadOp StencilLoadOp{ ATTACHMENT_LOAD_OP_DONT_CARE };
		RAttachmentStoreOp StencilStoreOp{ ATTACHMENT_STORE_OP_DONT_CARE };
		RSClear Clear;
	};

	struct RSubPassInfo {
		struct RAttachmentRef {
			uint32 Index;
			RImageLayout Layout;
		};
		RPipelineType Type;
		TVector<RAttachmentRef> InputAttachments;
		TVector<RAttachmentRef> ColorAttachments;
		RAttachmentRef DepthStencilAttachment;
	};

	struct RSubpassDependency {
		uint32 SrcSubPass;
		RPipelineStageFlags SrcStage;
		RAccessFlags SrcAccess;
		uint32 DstSubPass;
		RPipelineStageFlags DstStage;
		RAccessFlags DstAccess;
		RDependencyFlags DependencyFlags;
	};

	struct RSImageBlit {
		RImageAspectFlags srcAspect;
		uint32		  srcMipLevel;
		uint32		  srcBaseLayer;
		uint32		  srcLayerCount;
		UOffset2D		  srcOffsets[2];
		RImageAspectFlags dstAspect;
		uint32		  dstMipLevel;
		uint32		  dstBaseLayer;
		uint32		  dstLayerCount;
		UOffset2D		  dstOffsets[2];
	};

	struct RSSamplerInfo {
		RFilter magFilter;
		RFilter minFilter;
		RSamplerMipmapMode mipmapMode;
		RSamplerAddressMode addressModeU;
		RSamplerAddressMode addressModeV;
		RSamplerAddressMode addressModeW;
		float minLodBias{ 0.0f };
		bool anisotropyEnable{ false };
		float maxAnisotropy{0.0f};
		bool compareEnable{ false };
		RCompareOp compreOp;
		float minLod{ 0.0f };
		float maxLod{ 1000.0f }; // none
		RBorderColor borderColor{ BORDER_COLOR_FLOAT_OPAQUE_BLACK };
		bool unnormalizedCoordinates{ false };
	};

	struct RSDescriptorSetLayoutBinding {
		RDescriptorType      descriptorType;
		uint32              descriptorCount;
		RShaderStageFlags    stageFlags;
	};

	struct RSPushConstantRange {
		RShaderStageFlags stageFlags;
		uint32 offset;
		uint32 size;
	};

	// graphics pipeline create info
	struct RPipelineShaderInfo {

		RShaderStageFlagBits stage;
		TVector<char> code;
		const char* funcName;
	};

	struct RVertexInputBinding {
		uint32 binding;
		uint32 stride;
		RVertexInputRate inputRate;
	};

	struct RVertexInputAttribute
	{
		uint32 binding;
		RFormat format;
		uint32 offset;
	};

	struct RStencilOpState {
		RStencilOp failOp;
		RStencilOp passOp;
		RStencilOp depthFailOp;
		RCompareOp compareOp;
		uint32   compareMask;
		uint32   writeMask;
		uint32   reference;
	};

	struct RColorBlendAttachmentState {
		bool					blendEnable;
		RBlendFactor            srcColorBlendFactor{ BLEND_FACTOR_ZERO };
		RBlendFactor            dstColorBlendFactor{ BLEND_FACTOR_ZERO };
		RBlendOp                colorBlendOp{ BLEND_OP_ADD };
		RBlendFactor            srcAlphaBlendFactor{ BLEND_FACTOR_ZERO };
		RBlendFactor            dstAlphaBlendFactor{ BLEND_FACTOR_ZERO };
		RBlendOp                alphaBlendOp{ BLEND_OP_ADD };
		RColorComponentFlags    colorWriteMask{ COLOR_COMPONENT_ALL };
	};

	struct RGraphicsPipelineCreateInfo {

		// shader stages
		TVector<RPipelineShaderInfo> Shaders;

		// vertex input
		TVector<RVertexInputBinding> Bindings;
		TVector<RVertexInputAttribute> Attributes;

		// input assembly
		RPrimitiveTopology Topology{ PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
		bool PrimitiveRestartEnable{ false };

		// tessellation
		uint32 PatchControlPoints{0};

		// viewport
		RSViewport Viewport;
		URect Scissor;

		// rasterization
		bool DepthClampEnable;
		bool RasterizerDiscardEnable;
		RPolygonMode PolygonMode;
		RCullModeFlags CullMode;
		bool Clockwise{ false };
		bool DepthBiasEnable{ false };
		float DepthBiasConstantFactor{ 0.0f };
		float DepthBiasClamp{ 0.0f };
		float DepthBiasSlopeFactor{ 0.0f };
		float LineWidth{ 1.0f };

		// multi sample
		RSampleCountFlagBits RasterizationSamples{SAMPLE_COUNT_1_BIT};
		bool SampleShadingEnable{false};
		float MinSampleShading{0.0f};
		const RSampleMask* pSampleMask{nullptr};
		bool AlphaToCoverageEnable{false};
		bool AlphaToOneEnable{false};

		// depth stencil
		bool DepthTestEnable;
		bool DepthWriteEnable;
		RCompareOp DepthCompareOp{ COMPARE_OP_LESS };
		bool DepthBoundsTestEnable{false};
		bool StencilTestEnable {false};
		RStencilOpState FrontStencilOp;
		RStencilOpState BackStencilOp;
		float MinDepthBounds;
		float MaxDepthBounds;

		// color blend
		bool LogicOpEnable;
		RLogicOp LogicOp{ LOGIC_OP_COPY };
		TVector<RColorBlendAttachmentState> AttachmentStates;
		float BlendConstants[4];

		// dynamic
		TVector<RDynamicState> DynamicStates;
	};


	class RSampler;
	class RImageView;
	class RBuffer;

	// use for update descriptor set
	union RDescriptorInfo {
		struct {
			const RSampler* sampler;
			const RImageView* imageView;
		};

		struct {
			const RBuffer* buffer;
			uint64 offset;
			uint64 range;
		};
	};

}