#include "Objects/Public/StaticMesh.h"
#include "Objects/Public/DirectionalLight.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/RenderResource.h"
#include "Render/Public/DefaultResource.h"
#include "Asset/Public/AssetLoader.h"
#include "Render/Public/GlobalShader.h"

namespace {
	class MeshGBufferVS: public Render::GlobalShader {
		GLOBAL_SHADER_IMPLEMENT(MeshGBufferVS, "GBuffer.hlsl", "MainVS", EShaderStageFlags::Vertex);
		SHADER_PERMUTATION_BEGIN_SWITCH(INSTANCED, false);
		SHADER_PERMUTATION_END(INSTANCED);
	};

	class MeshGBufferPS : public Render::GlobalShader {
		GLOBAL_SHADER_IMPLEMENT(MeshGBufferPS, "GBuffer.hlsl", "MainPS", EShaderStageFlags::Pixel);
		SHADER_PERMUTATION_BEGIN_SWITCH(INSTANCED, false);
		SHADER_PERMUTATION_END(INSTANCED);
	};

	void InitializeMeshGBufferPSO(RHIGraphicsPipelineStateDesc& desc) {
		Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
		const TStaticArray<ERHIFormat, 2> colorFormats = { Object::RenderScene::GetGBufferNormalFormat(), Object::RenderScene::GetGBufferAlbedoFormat() };
		const ERHIFormat depthFormat = RHI::Instance()->GetDepthFormat();
		MeshGBufferVS::ShaderPermutation p; p.INSTANCED = false;
		desc.VertexShader = globalShaderMap->GetShader<MeshGBufferVS>(p)->GetRHI();
		desc.PixelShader = globalShaderMap->GetShader<MeshGBufferPS>()->GetRHI();
		// ds layout
		auto& layout = desc.Layout;
		layout.Resize(3);
		layout[0] = {
			{EBindingType::UniformBuffer, EShaderStageFlags::Vertex},
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
			{POSITION(0), 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
			{NORMAL(0), 1, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Normal)},//normal
			{TANGENT(0), 2, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Tangent)},//tangent
			{TEXCOORD(0), 3, 0, ERHIFormat::R32G32_SFLOAT, offsetof(Asset::AssetVertex, UV)},// uv
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

	void InitializeInstancedMeshGBufferPSO(RHIGraphicsPipelineStateDesc& desc) {
		Render::GlobalShaderMap* globalShaderMap = Render::GlobalShaderMap::Instance();
		MeshGBufferVS::ShaderPermutation vsp; vsp.INSTANCED = true;
		desc.VertexShader = globalShaderMap->GetShader<MeshGBufferVS>(vsp)->GetRHI();
		MeshGBufferPS::ShaderPermutation psp; psp.INSTANCED = true;
		desc.PixelShader = globalShaderMap->GetShader<MeshGBufferPS>(psp)->GetRHI();
		// ds layout
		desc.Layout = {
			{ {EBindingType::UniformBuffer, EShaderStageFlags::Vertex}},// camera
			{
				{EBindingType::Texture, EShaderStageFlags::Pixel}, // albedo tex
				{EBindingType::Sampler, EShaderStageFlags::Pixel} }
		};
		// vertex input
		auto& vi = desc.VertexInput;
		vi.Bindings = {
			{0, sizeof(Asset::AssetVertex), false},
			{1, sizeof(Math::FMatrix4x4), true}};
		vi.Attributes = {
			{POSITION(0), 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
			{NORMAL(0), 1, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Normal)},//normal
			{TANGENT(0), 2, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Tangent)},//tangent
			{TEXCOORD(0), 3, 0, ERHIFormat::R32G32_SFLOAT, offsetof(Asset::AssetVertex, UV)},// uv
			// instance transform
			{INSTANCE_TRANSFORM(0), 0, 1, ERHIFormat::R32G32B32A32_SFLOAT, 0},
			{INSTANCE_TRANSFORM(1), 0, 1, ERHIFormat::R32G32B32A32_SFLOAT, sizeof(Math::FVector4)},
			{INSTANCE_TRANSFORM(2), 0, 1, ERHIFormat::R32G32B32A32_SFLOAT, sizeof(Math::FVector4) * 2},
			{INSTANCE_TRANSFORM(3), 0, 1, ERHIFormat::R32G32B32A32_SFLOAT, sizeof(Math::FVector4) * 3},};

		desc.BlendDesc.BlendStates = { {false}, {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Back };
		desc.DepthStencilState = { true, true, ECompareType::Less, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;

		const TStaticArray<ERHIFormat, 2> colorFormats = { Object::RenderScene::GetGBufferNormalFormat(), Object::RenderScene::GetGBufferAlbedoFormat() };
		desc.NumColorTargets = colorFormats.Size();
		for (uint32 i = 0; i < colorFormats.Size(); ++i) {
			desc.ColorFormats[i] = colorFormats[i];
		}
		desc.DepthStencilFormat = RHI::Instance()->GetDepthFormat();
		desc.NumSamples = 1;
	}



	static const uint32 s_MeshGBufferPSOID{ Object::StaticPipelineStateMgr::RegisterPSOInitializer(InitializeMeshGBufferPSO) };
	static const uint32 s_InstancedMeshGBufferPSOID{ Object::StaticPipelineStateMgr::RegisterPSOInitializer(InitializeInstancedMeshGBufferPSO) };
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
		// calc aabb
		if(Primitives.Size()) {
			uint32 i = 0;
			AABB = Primitives[i++].AABB;
			for (; i < Primitives.Size(); ++i) {
				AABB.Union(Primitives[i].AABB);
			}
		}
		else {
			AABB = {};
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
					if(light->GetFrustum(i).Cull(aabb)) {
						RHIDynamicBuffer shadowUniform = light->GetShadowUniform(i);
						RHIGraphicsPipelineState* pso = light->GetCSMRenderingPSO();
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
	}

	void InstancedDataECSComponent::BuildInstances(const Math::AABB3& resAABB, TArray<Math::FTransform>&& instances) {
		InstanceData.Build(resAABB, MoveTemp(instances));
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
		const auto* meshCom = GetScene()->GetComponent<MeshECSComponent>(GetEntityID());
		if(!meshCom) {
			LOG_WARNING("[InstanceDataComponent::SetInstanceFile] Mesh not found!");
			return;
		}
		TArray<Math::FTransform> instances;
		Asset::InstancedMeshAsset::LoadInstanceFile(m_InstanceFile.c_str(), instances);
		auto* dataCom = GetScene()->GetComponent<InstancedDataECSComponent>(GetEntityID());
		dataCom->BuildInstances(meshCom->AABB, MoveTemp(instances));
	}

	void InstancedMeshRenderSystem::Update(ECSScene* ecsScene, MeshECSComponent* meshCom, InstancedDataECSComponent* instanceCom) {
		// create draw call
		Object::RenderScene* scene = (Object::RenderScene*)ecsScene;
		Object::RenderCamera* mainCamera = scene->GetMainCamera();
		Object::DirectionalLight* light = scene->GetDirectionalLight();
		// main camera
		{
			TArray<Math::FMatrix4x4> instances;
			instanceCom->InstanceData.Cull(mainCamera->GetFrustum(), instances);
			if(instances.Size()) {
				RHIDynamicBuffer instanceBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Index, instances.ByteSize(), instances.Data(), sizeof(Math::FMatrix4x4));
				RHIDynamicBuffer cameraUniform = mainCamera->GetUniformBuffer();
				uint32 instanceCount = instances.Size();
				for(auto& primitive: meshCom->Primitives) {
					scene->GetBasePasDrawCallQueue().PushDrawCall([&primitive, instanceBuffer, cameraUniform, instanceCount](RHICommandBuffer* cmd) {
						RHIGraphicsPipelineState* pso = StaticPipelineStateMgr::Instance()->GetGraphicsPipelineState(s_InstancedMeshGBufferPSOID);
						cmd->BindGraphicsPipeline(pso);
						cmd->BindVertexBuffer(primitive.VertexBuffer.Get(), 0, 0);
						cmd->BindVertexBuffer(instanceBuffer, 1, 0);
						cmd->BindIndexBuffer(primitive.IndexBuffer.Get(), 0);
						cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(cameraUniform));
						// material albedo
						cmd->SetShaderParam(1, 0, RHIShaderParam::Texture(primitive.Texture));
						RHISampler* defaultSampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
						cmd->SetShaderParam(1, 1, RHIShaderParam::Sampler(defaultSampler));
						cmd->DrawIndexed(primitive.IndexCount, instanceCount, 0, 0, 0);
					});
				}
			}
		}
		// cascade shadow
		if (RHIGraphicsPipelineState* pso = light->GetCSMInstancedRenderingPSO(); pso && light->GetEnableShadow()) {
			for(uint32 i=0; i<light->GetCascadeNum(); ++i) {
				TArray<Math::FMatrix4x4> instances;
				instanceCom->InstanceData.Cull(light->GetFrustum(i), instances);
				if(instances.Size()) {
					RHIDynamicBuffer shadowUniform = light->GetShadowUniform(i);
					RHIDynamicBuffer instanceBuffer = RHI::Instance()->AllocateDynamicBuffer(EBufferFlags::Vertex, instances.ByteSize(), instances.Data(), sizeof(Math::FMatrix4x4));
					uint32 instanceCount = instances.Size();
					for(auto& primitive: meshCom->Primitives) {
						light->GetDrawCallQueue(i).PushDrawCall([&primitive, pso, instanceBuffer, shadowUniform, instanceCount, this](RHICommandBuffer* cmd) {
							cmd->BindGraphicsPipeline(pso);
							cmd->BindVertexBuffer(primitive.VertexBuffer.Get(), 0, 0);
							cmd->BindVertexBuffer(instanceBuffer, 1, 0);
							cmd->BindIndexBuffer(primitive.IndexBuffer.Get(), 0);
							cmd->SetShaderParam(0, 0, RHIShaderParam::UniformBuffer(shadowUniform));
							cmd->DrawIndexed(primitive.IndexCount, instanceCount, 0, 0, 0);
						});
					}
				}
			}
		}
	}
}
