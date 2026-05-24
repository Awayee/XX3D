#include "Objects/Public/SkyBox.h"
#include "Asset/Public/MeshAsset.h"
#include "Asset/Public/AssetLoader.h"
#include "Render/Public/GlobalShader.h"
#include "Asset/Public/TextureAsset.h"
#include "Objects/Public/RenderCamera.h"
#include "Objects/Public/RenderResource.h"
#include "Render/Public/DefaultResource.h"

namespace {
	class SkyBoxVS: public Render::GlobalShader {
		SHADER_PERMUTATION_EMPTY();
		BEGIN_SHADER_BINDING
		SHADER_BINDING(0, UniformBuffer, uCamera, 1)
		END_SHADER_BINDING
		GLOBAL_SHADER_IMPLEMENT(SkyBoxVS, "SkyBox.hlsl", "MainVS", EShaderStageFlags::Vertex);
	};

	class SkyBoxPS: public Render::GlobalShader {
		SHADER_PERMUTATION_EMPTY();
		BEGIN_SHADER_BINDING
		SHADER_BINDING(1, Texture, inSkyBoxMap, 1)
		SHADER_BINDING(2, Sampler, inSampler, 1)
		END_SHADER_BINDING
		GLOBAL_SHADER_IMPLEMENT(SkyBoxPS, "SkyBox.hlsl", "MainPS", EShaderStageFlags::Pixel);
	};

	void CreateSkyBoxPSO(RHIGraphicsPipelineStateDesc& desc) {
		ERHIFormat colorFormat = RHI::Instance()->GetViewport()->GetBackBufferFormat();
		Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
		desc.VertexShader = globalShaderMap->GetShader<SkyBoxVS>()->GetRHI();
		desc.PixelShader = globalShaderMap->GetShader<SkyBoxPS>()->GetRHI();
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

	SkyBoxECSComp::SkyBoxECSComp(): Primitive(nullptr), CubeMap(nullptr){

		// Build primitive
		Asset::MeshAsset meshAsset;
		if (!Asset::AssetLoader::LoadProjectAsset(&meshAsset, BOX_FILE.c_str())) {
			LOG_WARNING("Failed to load skybox!");
			return;
		}
		for (auto& p : meshAsset.Primitives) {
			Primitive = Object::StaticResourceMgr::Instance()->GetPrimitive(p.BinaryFile);
			break;
		}
	}

	void SkyBoxECSComp::LoadCubeMap(const XString& file) {
		if(RHITexture* tex = StaticResourceMgr::Instance()->GetTexture(file)) {
			CubeMap = tex;
		}
		else {
			CubeMap = Render::DefaultResources::Instance()->GetDefaultTextureCube(Render::DefaultResources::TEX_WHITE);
		}
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
		if(nullptr == component->CubeMap) {
			return;
		}
		RenderScene* renderScene = (RenderScene*)scene;
		Render::DrawCallQueue& queue = renderScene->GetLightingPassDrawCallQueue();
		queue.PushDrawCall([renderScene, primitive=component->Primitive, cubemap=component->CubeMap](RHICommandBuffer* cmd) {
			RHIGraphicsPipelineState* pso = StaticResourceMgr::Instance()->GetGraphicsPipelineState(s_SkyBoxPSOID);
			cmd->BindGraphicsPipeline(pso);
			cmd->BindVertexBuffer(primitive->VertexBuffer, 0, 0);
			cmd->BindIndexBuffer(primitive->IndexBuffer, 0);
			cmd->SetShaderParam(SkyBoxVS::uCamera, RHIShaderParam::UniformBuffer(renderScene->GetMainCamera()->GetBuffer()));
			cmd->SetShaderParam(SkyBoxPS::inSkyBoxMap, RHIShaderParam::Texture(cubemap));
			auto sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
			cmd->SetShaderParam(SkyBoxPS::inSampler, RHIShaderParam::Sampler(sampler));
			cmd->DrawIndexed(primitive->IndexCount, 1, 0, 0, 0);
		});
	}
}
