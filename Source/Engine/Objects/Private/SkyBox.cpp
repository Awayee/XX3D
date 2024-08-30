#include "Objects/Public/SkyBox.h"
#include "Asset/Public/MeshAsset.h"
#include "Asset/Public/AssetLoader.h"
#include "Render/Public/GlobalShader.h"
#include "Asset/Public/TextureAsset.h"
#include "Render/Public/DefaultResource.h"

namespace {
	IMPLEMENT_GLOBAL_SHADER(SkyBoxVS, "SkyBox.hlsl", "MainVS", SHADER_STAGE_VERTEX_BIT);
	IMPLEMENT_GLOBAL_SHADER(SkyBoxPS, "SkyBox.hlsl", "MainPS", SHADER_STAGE_PIXEL_BIT);
}

namespace Object {

	static const XString BOX_FILE = "Meshes/Cube.mesh";

	SkyBox::SkyBox(Object::RenderCamera* camera): m_Camera(camera) {
		BuildPrimitive();
		CreatePSO();
	}

	SkyBox::~SkyBox() {
	}

	void SkyBox::ResetCubeMap(const XString& file) {
		m_CubeMap.Reset();
		RHITextureDesc desc = RHITextureDesc::TextureCube();
		Asset::TextureAsset asset;
		if (!Asset::AssetLoader::LoadProjectAsset(&asset, file.c_str())) {
			LOG_WARNING("Failed to load cube map file: %s", file.c_str());
			return;
		}
		if (asset.Type != Asset::ETextureAssetType::RGBA8_Cube) {
			LOG_WARNING("Texture is not a cube map! %s", file.c_str());
			return;
		}

		desc.Width = asset.Width;
		desc.Height = asset.Height;
		desc.Format = ERHIFormat::B8G8R8A8_UNORM;
		desc.Flags = TEXTURE_FLAG_SRV | TEXTURE_FLAG_CPY_DST;
		m_CubeMap = RHI::Instance()->CreateTexture(desc);
		m_CubeMap->UpdateData(asset.Pixels.Size(), asset.Pixels.Data(), {});
	}

	void SkyBox::ResetCubeMap() {
		m_CubeMap.Reset();
	}

	void SkyBox::CreateDrawCall(Render::DrawCallQueue& dcQueue) {
		dcQueue.PushDrawCall([this](RHICommandBuffer* cmd) {
			cmd->BindGraphicsPipeline(m_PSO);
			cmd->BindIndexBuffer(m_IndexBuffer, 0);
			cmd->BindVertexBuffer(m_VertexBuffer, 0, 0);
			cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(m_Camera->GetBuffer()));
			RHITexture* cubeTex = m_CubeMap.Get();
			if(!cubeTex) {
				cubeTex = Render::DefaultResources::Instance()->GetDefaultTextureCube(Render::DefaultResources::TEX_WHITE);
			}
			cmd->SetShaderParam(0, 1, RHIShaderParam::Texture(cubeTex));
			auto sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
			cmd->SetShaderParam(0, 2, RHIShaderParam::Sampler(sampler));
			cmd->DrawIndexed(m_IndexCount, 1, 0, 0, 0);
		});
	}

	void SkyBox::BuildPrimitive() {
		Asset::MeshAsset meshAsset;
		if (!Asset::AssetLoader::LoadProjectAsset(&meshAsset, BOX_FILE.c_str())) {
			LOG_WARNING("Failed to load skybox!");
			return;
		}
		for(auto& p: meshAsset.Primitives) {
			m_VertexCount = p.Vertices.Size();
			m_IndexCount = p.Indices.Size();
			m_VertexBuffer = RHI::Instance()->CreateBuffer({ BUFFER_FLAG_VERTEX | BUFFER_FLAG_COPY_DST, p.Vertices.ByteSize(), 0 });
			m_VertexBuffer->UpdateData(p.Vertices.Data(), p.Vertices.ByteSize(), 0);
			m_IndexBuffer = RHI::Instance()->CreateBuffer({ BUFFER_FLAG_INDEX | BUFFER_FLAG_COPY_DST, p.Indices.ByteSize(), 0 });
			m_IndexBuffer->UpdateData(p.Indices.Data(), p.Indices.ByteSize(), 0);
			break;
		}
	}

	void SkyBox::CreatePSO() {
		ERHIFormat colorFormat = RHI::Instance()->GetViewport()->GetBackBufferFormat();
		RHIGraphicsPipelineStateDesc desc{};
		Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
		desc.VertexShader = globalShaderMap->GetShader<SkyBoxVS>()->GetRHI();
		desc.PixelShader = globalShaderMap->GetShader<SkyBoxPS>()->GetRHI();
		// ds layout
		auto& layout = desc.Layout;
		layout.Resize(1);
		layout[0] = {
			{EBindingType::UniformBuffer, SHADER_STAGE_VERTEX_BIT}, // camera
			{EBindingType::Texture, SHADER_STAGE_PIXEL_BIT},// cube map
			{EBindingType::Sampler, SHADER_STAGE_PIXEL_BIT}
		};
		// vertex input
		auto& vi = desc.VertexInput;
		vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
		vi.Attributes = {
			{0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
		};

		desc.BlendDesc.BlendStates = { {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Null }; // do not cull
		desc.DepthStencilState = { true, ECompareType::LessEqual, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
		desc.ColorFormats.PushBack(colorFormat);
		desc.ColorFormats[0] = colorFormat;
		desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
		desc.NumSamples = 1;
		m_PSO = RHI::Instance()->CreateGraphicsPipelineState(desc);
	}

}
