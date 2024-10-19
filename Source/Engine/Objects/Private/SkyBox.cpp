#include "Objects/Public/SkyBox.h"
#include "Asset/Public/MeshAsset.h"
#include "Asset/Public/AssetLoader.h"
#include "Render/Public/GlobalShader.h"
#include "Asset/Public/TextureAsset.h"
#include "Objects/Public/RenderResource.h"
#include "Render/Public/DefaultResource.h"

namespace {
	class SkyBoxVS: public Render::GlobalShader {
		GLOBAL_SHADER_IMPLEMENT(SkyBoxVS, "SkyBox.hlsl", "MainVS", EShaderStageFlags::Vertex);
		SHADER_PERMUTATION_EMPTY();
	};

	class SkyBoxPS: public Render::GlobalShader {
		GLOBAL_SHADER_IMPLEMENT(SkyBoxPS, "SkyBox.hlsl", "MainPS", EShaderStageFlags::Pixel);
		SHADER_PERMUTATION_EMPTY();
	};

	void CreateSkyBoxPSO(RHIGraphicsPipelineStateDesc& desc) {
		ERHIFormat colorFormat = RHI::Instance()->GetViewport()->GetBackBufferFormat();
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
			{POSITION(0), 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
		};
		desc.BlendDesc.BlendStates = { {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Null }; // do not cull
		desc.DepthStencilState = { true, true, ECompareType::LessEqual, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
		desc.NumColorTargets = 1;
		desc.ColorFormats[0] = colorFormat;
		desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
		desc.NumSamples = 1;
	}

	static const uint32 s_SkyBoxPSOID{ Object::StaticResourceMgr::Instance()->RegisterPSOInitializer(CreateSkyBoxPSO) };
}

namespace Object {

	static const XString BOX_FILE = "Meshes/Cube.mesh";

	SkyBoxECSComp::SkyBoxECSComp() : VertexCount(0), IndexCount(0){

		// Build primitive
		Asset::MeshAsset meshAsset;
		if (!Asset::AssetLoader::LoadProjectAsset(&meshAsset, BOX_FILE.c_str())) {
			LOG_WARNING("Failed to load skybox!");
			return;
		}
		for (auto& p : meshAsset.Primitives) {
			auto primitive = Object::StaticResourceMgr::Instance()->GetPrimitive(p.BinaryFile);
			VertexBuffer = primitive.VertexBuffer;
			IndexBuffer = primitive.IndexBuffer;
			VertexCount = primitive.VertexCount;
			IndexCount = primitive.IndexCount;
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
		if (asset.Type != Asset::ETextureAssetType::RGBA8Srgb_Cube) {
			LOG_WARNING("Texture is not a cube map! %s", file.c_str());
			return;
		}

		RHITextureDesc desc = RHITextureDesc::TextureCube();
		desc.Width = asset.Width;
		desc.Height = asset.Height;
		desc.Format = Object::TextureAssetTypeToRHIFormat(asset.Type);
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
			RHIGraphicsPipelineState* pso = StaticResourceMgr::Instance()->GetGraphicsPipelineState(s_SkyBoxPSOID);
			cmd->BindGraphicsPipeline(pso);
			cmd->BindVertexBuffer(component->VertexBuffer, 0, 0);
			cmd->BindIndexBuffer(component->IndexBuffer, 0);
			cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(renderScene->GetMainCamera()->GetUniformBuffer()));
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
