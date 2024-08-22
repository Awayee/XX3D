#include "Render/Public/MeshRender.h"
#include "Render/Public/GlobalShader.h"
#include "Math/Public/Math.h"

namespace Render {
	IMPLEMENT_GLOBAL_SHADER(GBufferVS, "GBuffer.hlsl", "MainVS", SHADER_STAGE_VERTEX_BIT);
	IMPLEMENT_GLOBAL_SHADER(GBufferPS, "GBuffer.hlsl", "MainPS", SHADER_STAGE_PIXEL_BIT);

	IMPLEMENT_GLOBAL_SHADER(DeferredLightingVS, "DeferredLightingPBR.hlsl", "MainVS", SHADER_STAGE_VERTEX_BIT);
	IMPLEMENT_GLOBAL_SHADER(DeferredLightingPS, "DeferredLightingPBR.hlsl", "MainPS", SHADER_STAGE_PIXEL_BIT);


	enum EMeshRenderSet {
		MESH_SET_SCENE = 0,
		MESH_SET_OBJECT = 1,
		MESH_SET_MATERIAL = 2,
	};

	struct FVertex {
		Math::FVector3 Position;
		Math::FVector3 Normal;
		Math::FVector3 Tangent;
		Math::FVector2 UV;
	};

	RHIGraphicsPipelineStatePtr CreateMeshGBufferRenderPSO(TConstArrayView<ERHIFormat> colorFormats, ERHIFormat depthFormat) {
		// Create PSO
		RHIGraphicsPipelineStateDesc desc{};
		GlobalShaderMap* globalShaderMap = GlobalShaderMap::Instance();
		desc.VertexShader = globalShaderMap->GetShader<GBufferVS>()->GetRHI();
		desc.PixelShader = globalShaderMap->GetShader<GBufferPS>()->GetRHI();
		// ds layout
		auto& layout = desc.Layout;
		layout.Resize(3);
		layout[MESH_SET_SCENE] = {
			{EBindingType::UniformBuffer, SHADER_STAGE_VERTEX_BIT | SHADER_STAGE_PIXEL_BIT},
		};
		layout[MESH_SET_OBJECT] = { {EBindingType::UniformBuffer, SHADER_STAGE_VERTEX_BIT} };
		layout[MESH_SET_MATERIAL] = {
			{EBindingType::Texture, SHADER_STAGE_PIXEL_BIT},
			{EBindingType::Sampler, SHADER_STAGE_PIXEL_BIT},
		};
		// vertex input
		auto& vertexInput = desc.VertexInput;
		vertexInput.Bindings = { {0, sizeof(FVertex), false} };
		vertexInput.Attributes = {
			{0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
			{1, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(FVertex, Normal)},//normal
			{2, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(FVertex, Tangent)},//tangent
			{3, 0, ERHIFormat::R32G32_SFLOAT, offsetof(FVertex, UV)}
		};

		desc.BlendDesc.BlendStates={{false}, {false}};
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Null };
		desc.DepthStencilState = { true, ECompareType::Less, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
		desc.ColorFormats.Resize(colorFormats.Size());
		for(uint32 i=0; i<colorFormats.Size(); ++i) {
			desc.ColorFormats[i] = colorFormats[i];
		}
		desc.DepthStencilFormat = depthFormat;
		desc.NumSamples = 1;
		return RHI::Instance()->CreateGraphicsPipelineState(desc);
	}

	RHIGraphicsPipelineStatePtr CreateDeferredLightingPSO(ERHIFormat colorFormat) {
		RHIGraphicsPipelineStateDesc desc{};
		GlobalShaderMap* globalShaderMap = GlobalShaderMap::Instance();
		desc.VertexShader = globalShaderMap->GetShader<DeferredLightingVS>()->GetRHI();
		desc.PixelShader = globalShaderMap->GetShader<DeferredLightingPS>()->GetRHI();
		auto& layout = desc.Layout;
		layout.Resize(1);
		layout[0] = {
			{EBindingType::UniformBuffer, SHADER_STAGE_PIXEL_BIT},// camera
			{EBindingType::UniformBuffer, SHADER_STAGE_PIXEL_BIT},// light
			{EBindingType::Texture, SHADER_STAGE_PIXEL_BIT},// normal tex
			{EBindingType::Texture, SHADER_STAGE_PIXEL_BIT},// albedo tex
			{EBindingType::Texture, SHADER_STAGE_PIXEL_BIT},// depth tex
			{EBindingType::Sampler, SHADER_STAGE_PIXEL_BIT},// sampler
		};
		desc.VertexInput = {};
		desc.BlendDesc.BlendStates = { {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Back };
		desc.DepthStencilState = { true, ECompareType::Less, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
		desc.ColorFormats.PushBack(colorFormat);
		desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
		desc.NumSamples = 1;
		return RHI::Instance()->CreateGraphicsPipelineState(desc);
	}
}
