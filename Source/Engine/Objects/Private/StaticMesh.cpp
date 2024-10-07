#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/RenderResource.h"
#include "Render/Public/DefaultResource.h"
#include "Asset/Public/AssetLoader.h"
#include "Render/Public/GlobalShader.h"

namespace {
	IMPLEMENT_GLOBAL_SHADER(MeshGBufferVS, "GBuffer.hlsl", "MainVS", EShaderStageFlags::Vertex);
	IMPLEMENT_GLOBAL_SHADER(MeshGBufferPS, "GBuffer.hlsl", "MainPS", EShaderStageFlags::Pixel);
	IMPLEMENT_GLOBAL_SHADER(DirectionalShadowVS, "DirectionalShadow.hlsl", "MainVS", EShaderStageFlags::Vertex);
	IMPLEMENT_GLOBAL_SHADER(DirectionalShadowPS, "DirectionalShadow.hlsl", "MainPS", EShaderStageFlags::Pixel);

	void InitializeMeshGBufferPSO(RHIGraphicsPipelineStateDesc& desc) {
		Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
		const TStaticArray<ERHIFormat, 2> colorFormats = { Object::RenderScene::GetGBufferNormalFormat(), Object::RenderScene::GetGBufferAlbedoFormat() };
		const ERHIFormat depthFormat = RHI::Instance()->GetDepthFormat();
		desc.VertexShader = globalShaderMap->GetShader<MeshGBufferVS>()->GetRHI();
		desc.PixelShader = globalShaderMap->GetShader<MeshGBufferPS>()->GetRHI();
		// ds layout
		auto& layout = desc.Layout;
		layout.Resize(3);
		layout[0] = {
			{EBindingType::UniformBuffer, EShaderStageFlags::Vertex | EShaderStageFlags::Pixel},
		};
		layout[1] = {
			{EBindingType::UniformBuffer, EShaderStageFlags::Vertex}
		};
		layout[2] = {
			{EBindingType::Texture, EShaderStageFlags::Pixel},
			{EBindingType::Sampler, EShaderStageFlags::Pixel},
		};
		// vertex input
		auto& vi = desc.VertexInput;
		vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
		vi.Attributes = {
			{POSITION, 0, 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
			{NORMAL, 0, 1, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Normal)},//normal
			{TANGENT, 0, 2, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Tangent)},//tangent
			{TEXCOORD, 0, 3, 0, ERHIFormat::R32G32_SFLOAT, offsetof(Asset::AssetVertex, UV)},// uv
		};

		desc.BlendDesc.BlendStates = { {false}, {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Back };
		desc.DepthStencilState = { true, true, ECompareType::Less, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
		desc.NumColorTargets = colorFormats.Size();
		for (uint32 i = 0; i < colorFormats.Size(); ++i) {
			desc.ColorFormats[i] = colorFormats[i];
		}
		desc.DepthStencilFormat = depthFormat;
		desc.NumSamples = 1;
	}

	void InitializeMeshDirectionalShadowPSO(RHIGraphicsPipelineStateDesc& desc) {
		// shader
		Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
		desc.VertexShader = globalShaderMap->GetShader<DirectionalShadowVS>()->GetRHI();
		desc.PixelShader = globalShaderMap->GetShader<DirectionalShadowPS>()->GetRHI();
		// layout
		auto& layout = desc.Layout;
		layout.Resize(2);
		layout[0] = { {EBindingType::UniformBuffer, EShaderStageFlags::Vertex} };// camera
		layout[1] = { {EBindingType::UniformBuffer, EShaderStageFlags::Vertex} };// mesh
		// vertex input
		auto& vi = desc.VertexInput;
		vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
		vi.Attributes = { {POSITION, 0, 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0} };

		desc.BlendDesc.BlendStates = { {false}, {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Null, false, 3.0f, 3.0f };
		desc.DepthStencilState = { true, true, ECompareType::Less, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
		desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
		desc.NumSamples = 1;
	}

	static const uint32 s_MeshGBufferPSOID{ Object::StaticPipelineStateMgr::RegisterPSOInitializer(InitializeMeshGBufferPSO) };
}

namespace Object {
	void TransformECSComponent::SetTransform(const Math::FTransform& transform) {
		m_Transform = transform;
		UpdateMat();
	}

	void TransformECSComponent::UpdateMat() {
		m_Transform.BuildMatrix(m_MatrixData.Transform);
		m_Transform.BuildInverseMatrix(m_MatrixData.InverseTransform);
	}

	void TransformComponent::OnLoad(const Json::Value& val) {
		Json::LoadFloatArray(val["Position"], Transform.Position.Data(), 3);
		Json::LoadFloatArray(val["Scale"], Transform.Scale.Data(), 3);
		Math::FVector3 euler;
		Json::LoadFloatArray(val["Rotation"], euler.Data(), 3);
		Transform.Rotation = Math::FQuaternion::Euler(euler);
		TransformUpdated();
	}

	void TransformComponent::OnAdd() {
		GetScene()->AddComponent<TransformECSComponent>(GetEntityID());
	}

	void TransformComponent::OnRemove() {
		GetScene()->RemoveComponent<TransformECSComponent>(GetEntityID());
	}

	void TransformComponent::TransformUpdated() {
		TransformECSComponent* component = GetScene()->GetComponent<TransformECSComponent>(GetEntityID());
		component->SetTransform(Transform);
	}

	void MeshECSComponent::BuildFromAsset(const Asset::MeshAsset& meshAsset) {
		auto r = RHI::Instance();
		Primitives.Reset();
		Primitives.Reserve(meshAsset.Primitives.Size());
		for(auto& srcPrimitive: meshAsset.Primitives) {
			auto& primitive = Primitives.EmplaceBack();
			primitive.VertexCount = srcPrimitive.Vertices.Size();
			primitive.IndexCount = srcPrimitive.Indices.Size();
			// vb
			RHIBufferDesc vbDesc{EBufferFlags::Vertex | EBufferFlags::CopyDst, srcPrimitive.Vertices.ByteSize(), sizeof(Asset::AssetVertex) };
			primitive.VertexBuffer = r->CreateBuffer(vbDesc);
			primitive.VertexBuffer->UpdateData(srcPrimitive.Vertices.Data(), srcPrimitive.Vertices.ByteSize(), 0);
			// ib
			RHIBufferDesc ibDesc{EBufferFlags::Index | EBufferFlags::CopyDst, srcPrimitive.Indices.ByteSize(), sizeof(Asset::IndexType) };
			primitive.IndexBuffer = r->CreateBuffer(ibDesc);
			primitive.IndexBuffer->UpdateData(srcPrimitive.Indices.Data(), srcPrimitive.Indices.ByteSize(), 0);
			// texture
			primitive.Texture = TextureResourceMgr::Instance()->GetTexture(srcPrimitive.MaterialFile);
			// aabb
			primitive.AABB = srcPrimitive.AABB;
		}
	}

	void MeshComponent::OnLoad(const Json::Value& val) {
		if(val.HasMember("MeshFile")) {
			SetMeshFile(val["MeshFile"].GetString());
		}
	}

	void MeshComponent::OnAdd() {
		GetScene()->AddComponent<MeshECSComponent>(GetEntityID());
	}

	void MeshComponent::OnRemove() {
		GetScene()->RemoveComponent<MeshECSComponent>(GetEntityID());
	}

	void MeshComponent::SetMeshFile(const XString& meshFile) {
		m_MeshFile = meshFile;
		MeshECSComponent* component = GetScene()->GetComponent<MeshECSComponent>(GetEntityID());
		Asset::MeshAsset asset;
		Asset::AssetLoader::LoadProjectAsset(&asset, m_MeshFile.c_str());
		component->BuildFromAsset(asset);
	}

	MeshRenderSystem::MeshRenderSystem() {
		RHIGraphicsPipelineStateDesc desc;
		InitializeMeshDirectionalShadowPSO(desc);
		m_DirectionalShadowPSO = RHI::Instance()->CreateGraphicsPipelineState(desc);
	}

	void MeshRenderSystem::PreUpdate(ECSScene* ecsScene) {
		// check and update shadow pso
		Object::RenderScene* scene = (Object::RenderScene*)ecsScene;
		const Object::DirectionalLight* light = scene->GetDirectionalLight();
		const auto& cfg = light->GetShadowConfig();
		const auto& desc = m_DirectionalShadowPSO->GetDesc();
		const auto& rasterizerState = desc.RasterizerState;
		if(Math::FloatEqual(rasterizerState.DepthBiasConstant, cfg.ShadowBiasConstant) && Math::FloatEqual(rasterizerState.DepthBiasSlope, cfg.ShadowBiasSlope)) {
			return;
		}
		auto newDesc = desc;
		newDesc.RasterizerState.DepthBiasConstant = cfg.ShadowBiasConstant;
		newDesc.RasterizerState.DepthBiasSlope = cfg.ShadowBiasSlope;
		m_DirectionalShadowPSO = RHI::Instance()->CreateGraphicsPipelineState(newDesc);
	}

	void MeshRenderSystem::Update(ECSScene* ecsScene, TransformECSComponent* transform, MeshECSComponent* staticMesh) {
		// update uniform
		RHIDynamicBuffer meshUniform = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Uniform, sizeof(TransformECSComponent::MatrixData), &transform->GetMatrixData(), 0);
		// create draw call
		Object::RenderScene* scene = (Object::RenderScene*)ecsScene;
		Object::RenderCamera* mainCamera = scene->GetMainCamera();
		Object::DirectionalLight* light = scene->GetDirectionalLight();
		for(auto& primitive: staticMesh->Primitives) {
			const Math::AABB3 aabb = primitive.AABB.Transform(transform->GetMatrixData().Transform);
			if(mainCamera->GetFrustum().Cull(aabb)) {
				RHIDynamicBuffer cameraUniform = mainCamera->GetUniformBuffer();
				scene->GetBasePasDrawCallQueue().PushDrawCall([&primitive, meshUniform, cameraUniform](RHICommandBuffer* cmd) {
					RHIGraphicsPipelineState* pso = StaticPipelineStateMgr::Instance()->GetGraphicsPipelineState(s_MeshGBufferPSOID);
					cmd->BindGraphicsPipeline(pso);
					cmd->BindVertexBuffer(primitive.VertexBuffer.Get(), 0, 0);
					cmd->BindIndexBuffer(primitive.IndexBuffer.Get(), 0);
					cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraUniform));
					cmd->SetShaderParam(1, 0, RHIShaderParam::UniformBuffer(meshUniform));
					// material albedo
					cmd->SetShaderParam(2, 0, RHIShaderParam::Texture(primitive.Texture));
					RHISampler* defaultSampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
					cmd->SetShaderParam(2, 1, RHIShaderParam::Sampler(defaultSampler));
					cmd->DrawIndexed(primitive.IndexCount, 1, 0, 0, 0);
				});
			}
			// for shadow map
			if(light->GetEnableShadow()) {
				for (uint32 i = 0; i < light->GetCascadeNum(); ++i) {
					const auto& frustum = light->GetFrustum(i);
					if(frustum.Cull(aabb)) {
						RHIDynamicBuffer shadowUniform = light->GetShadowUniform(i);
						RHIGraphicsPipelineState* pso = m_DirectionalShadowPSO.Get();
						light->GetDrawCallQueue(i).PushDrawCall([&primitive, pso, shadowUniform, meshUniform](RHICommandBuffer* cmd) {
							cmd->BindGraphicsPipeline(pso);
							cmd->BindVertexBuffer(primitive.VertexBuffer.Get(), 0, 0);
							cmd->BindIndexBuffer(primitive.IndexBuffer.Get(), 0);
							cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(shadowUniform));
							cmd->SetShaderParam(1, 0, RHIShaderParam::UniformBuffer(meshUniform));
							cmd->DrawIndexed(primitive.IndexCount, 1, 0, 0, 0);
						});
					}
				}
			}
		}
	}

	void InstancedDataECSComponent::BuildInstances(const XString& instanceFile) {
		Asset::InstancedMeshAsset::LoadInstanceFile(instanceFile.c_str(), Instances);
	}

	void InstanceDataComponent::OnLoad(const Json::Value& val) {
		if(val.HasMember("InstanceFile")) {
			SetInstanceFile(val["InstanceFile"].GetString());
		}
	}

	void InstanceDataComponent::OnAdd() {
		GetScene()->AddComponent<InstancedDataECSComponent>(GetEntityID());
	}

	void InstanceDataComponent::OnRemove() {
		GetScene()->RemoveComponent<InstancedDataECSComponent>(GetEntityID());
	}

	void InstanceDataComponent::SetInstanceFile(const XString& file) {
		m_InstanceFile = file;
		auto* com = GetScene()->GetComponent<InstancedDataECSComponent>(GetEntityID());
		com->BuildInstances(file);
	}

	void InstancedMeshRenderSystem::Update(ECSScene* scene, MeshECSComponent* meshCom, InstancedDataECSComponent* instanceCom) {

	}
}
