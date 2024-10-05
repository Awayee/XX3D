#include "Objects/Public/SkyBox.h"
#include "Asset/Public/MeshAsset.h"
#include "Asset/Public/AssetLoader.h"
#include "Render/Public/GlobalShader.h"
#include "Asset/Public/TextureAsset.h"
#include "Objects/Public/TextureResource.h"
#include "Render/Public/DefaultResource.h"

namespace {
	IMPLEMENT_GLOBAL_SHADER(SkyBoxVS, "SkyBox.hlsl", "MainVS", EShaderStageFlags::Vertex);
	IMPLEMENT_GLOBAL_SHADER(SkyBoxPS, "SkyBox.hlsl", "MainPS", EShaderStageFlags::Pixel);
}

namespace Object {

	static const XString BOX_FILE = "Meshes/Cube.mesh";

	SkyBoxECSComp::SkyBoxECSComp() : VertexCount(0), IndexCount(0){
		// Create PSO
		ERHIFormat colorFormat = RHI::Instance()->GetViewport()->GetBackBufferFormat();
		RHIGraphicsPipelineStateDesc desc{};
		Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
		desc.VertexShader = globalShaderMap->GetShader<SkyBoxVS>()->GetRHI();
		desc.PixelShader = globalShaderMap->GetShader<SkyBoxPS>()->GetRHI();
		// ds layout
		auto& layout = desc.Layout;
		layout.Resize(1);
		layout[0] = {
			{EBindingType::UniformBuffer, EShaderStageFlags::Vertex}, // camera
			{EBindingType::Texture, EShaderStageFlags::Pixel},// cube map
			{EBindingType::Sampler, EShaderStageFlags::Pixel}
		};
		// vertex input
		auto& vi = desc.VertexInput;
		vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
		vi.Attributes = {
			{POSITION, 0, 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
		};

		desc.BlendDesc.BlendStates = { {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Null }; // do not cull
		desc.DepthStencilState = { true, true, ECompareType::LessEqual, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
		desc.NumColorTargets = 1;
		desc.ColorFormats[0] = colorFormat;
		desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
		desc.NumSamples = 1;
		PSO = RHI::Instance()->CreateGraphicsPipelineState(desc);

		// Build primitive
		Asset::MeshAsset meshAsset;
		if (!Asset::AssetLoader::LoadProjectAsset(&meshAsset, BOX_FILE.c_str())) {
			LOG_WARNING("Failed to load skybox!");
			return;
		}
		for (auto& p : meshAsset.Primitives) {
			VertexCount = p.Vertices.Size();
			IndexCount = p.Indices.Size();
			VertexBuffer = RHI::Instance()->CreateBuffer({ EBufferFlags::Vertex | EBufferFlags::CopyDst, p.Vertices.ByteSize(), sizeof(Asset::AssetVertex) });
			VertexBuffer->UpdateData(p.Vertices.Data(), p.Vertices.ByteSize(), 0);
			IndexBuffer = RHI::Instance()->CreateBuffer({ EBufferFlags::Index | EBufferFlags::CopyDst, p.Indices.ByteSize(), sizeof(Asset::IndexType) });
			IndexBuffer->UpdateData(p.Indices.Data(), p.Indices.ByteSize(), 0);
			break;
		}
	}

	void SkyBoxECSComp::LoadCubeMap(const XString& file) {
		CubeMap.Reset();
		if (file.empty()) {
			return;
		}
		Asset::TextureAsset asset;
		if (!Asset::AssetLoader::LoadProjectAsset(&asset, file.c_str())) {
			LOG_WARNING("Failed to load cube map file: %s", file.c_str());
			return;
		}
		if (asset.Type != Asset::ETextureAssetType::RGBA8_Cube) {
			LOG_WARNING("Texture is not a cube map! %s", file.c_str());
			return;
		}

		RHITextureDesc desc = RHITextureDesc::TextureCube();
		desc.Width = asset.Width;
		desc.Height = asset.Height;
		desc.Format = Object::TextureResource::ConvertTextureFormat(asset.Type);
		desc.Flags = (ETextureFlags::SRV | ETextureFlags::CopyDst);
		CubeMap = RHI::Instance()->CreateTexture(desc);
		CubeMap->UpdateData(asset.Pixels.Size(), asset.Pixels.Data());
	}

	void SkyBoxComponent::OnLoad(const Json::Value& val) {
		if(val.HasMember("CubeMapFile")) {
			SetCubeMapFile(val["CubeMapFile"].GetString());
		}
	}

	void SkyBoxComponent::OnAdd() {
		GetScene()->AddComponent<SkyBoxECSComp>(GetEntityID());
	}

	void SkyBoxComponent::OnRemove() {
		GetScene()->RemoveComponent<SkyBoxECSComp>(GetEntityID());
	}

	void SkyBoxComponent::SetCubeMapFile(const XString& file) {
		m_CubeMapFile = file;
		auto* com = GetScene()->GetComponent<SkyBoxECSComp>(GetEntityID());
		com->LoadCubeMap(file);
	}

	void SkyBoxSystem::Update(ECSScene* scene, SkyBoxECSComp* component) {
		RenderScene* renderScene = (RenderScene*)scene;
		Render::DrawCallQueue& queue = renderScene->GetLightingPassDrawCallQueue();
		queue.PushDrawCall([renderScene, component](RHICommandBuffer* cmd) {
			cmd->BindGraphicsPipeline(component->PSO);
			cmd->BindVertexBuffer(component->VertexBuffer, 0, 0);
			cmd->BindIndexBuffer(component->IndexBuffer, 0);
			cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(renderScene->GetMainCamera()->GertUniformBuffer()));
			RHITexture* cubeTex = component->CubeMap.Get();
			if (!cubeTex) {
				cubeTex = Render::DefaultResources::Instance()->GetDefaultTextureCube(Render::DefaultResources::TEX_WHITE);
			}
			cmd->SetShaderParam(0, 1, RHIShaderParam::Texture(cubeTex));
			auto sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
			cmd->SetShaderParam(0, 2, RHIShaderParam::Sampler(sampler));
			cmd->DrawIndexed(component->IndexCount, 1, 0, 0, 0);
		});
	}
}
