#include "Objects/Public/Material.h"
#include "Objects/Public/InstanceDataMgr.h"
#include "Objects/Public/RenderResource.h"
#include "RHI/Public/RHI.h"
#include "Core/Public/File.h"
#include "Render/Public/ShaderCompiler.h"
#include "Render/Public/DefaultResource.h"
#include "Core/Public/Algorithm.h"
#include "Asset/Public/AssetLoader.h"

#define BASE_PASS_TEMPLATE_SHADER "BasePassTemplate.hlsl"

namespace Object {

	using EParameterType = Asset::EMaterialParamType;

	inline XString ConvertAssignmentStr(Asset::EMaterialParamAssign assignment) {
		switch (assignment) {
		case Asset::EMaterialParamAssign::Equal: return "=";
		case Asset::EMaterialParamAssign::Add: return "+=";
		case Asset::EMaterialParamAssign::Multiply: return "*=";
		}
		CHECK(0);
	}

	inline XString ConvertOutputStr(Asset::EMaterialParamOutput output) {
		switch (output) {
		case Asset::EMaterialParamOutput::X: return "x";
		case Asset::EMaterialParamOutput::XY: return "xy";
		case Asset::EMaterialParamOutput::XYZ: return "xyz";
		case Asset::EMaterialParamOutput::XYZW: return "xyzw";
		}
		CHECK(0);
	}

	inline uint32 GetMaterialSetIndex(bool bInstanced) {
		return bInstanced ? 1 : 2;
	}

	MaterialTemplate::MaterialTemplate(const Asset::MaterialTemplateAsset& asset) {
		Reload(asset);
	}

	MaterialTemplate::MaterialTemplate(MaterialTemplate&& rhs) noexcept :
	m_Vectors(MoveTemp(rhs.m_Vectors)),
	m_Textures(MoveTemp(rhs.m_Textures)),
	m_Samplers(MoveTemp(rhs.m_Samplers)),
	m_MaterialOutputs(MoveTemp(rhs.m_MaterialOutputs)),
	m_Shader(MoveTemp(rhs.m_Shader)),
	m_InstancedShader(MoveTemp(rhs.m_InstancedShader)),
	m_MaterialCB(MoveTemp(rhs.m_MaterialCB)),
	m_Hash(rhs.m_Hash){}

	void MaterialTemplate::FillBasePassPSODesc(RHIGraphicsPipelineStateDesc& desc, bool bInstanced) {
		// shader
		auto& shader = GetOrCreateShader(bInstanced);
		desc.VertexShader = shader.VS.Get();
		desc.PixelShader = shader.PS.Get();
		// layout
		const uint32 materialSetIdx = GetMaterialSetIndex(bInstanced);
		desc.Layout.Resize(materialSetIdx + 1);
		// scene set
		desc.Layout[0] = {
			{EBindingType::UniformBuffer, EShaderStageFlags::Vertex},
		};
		// object set if not instanced
		if(!bInstanced) {
			desc.Layout[1] = {
				{EBindingType::UniformBuffer, EShaderStageFlags::Vertex}
			};
		}
		// material set
		auto& materialSet = desc.Layout[materialSetIdx];
		const uint32 bindingSize = 1 + m_Textures.Size() + m_Samplers.Size();
		materialSet.Reserve(bindingSize);
		if (m_Vectors.Size()) {
			auto& dst = materialSet.EmplaceBack();
			dst.Type = EBindingType::UniformBuffer;
			dst.StageFlags = EShaderStageFlags::Pixel;
			dst.Count = 1;
		}
		for (auto& texInfo : m_Textures) {
			materialSet.PushBack({ EBindingType::Texture, EShaderStageFlags::Pixel, 1 });
		}
		for (auto& sampler : m_Samplers) {
			materialSet.PushBack({ EBindingType::Sampler, EShaderStageFlags::Pixel, 1 });
		}
		// vertex input;
		auto& vi = desc.VertexInput;
		if(bInstanced){
			const ERHIFormat instanceRowFormat = Object::GetInstanceDataRowFormat();
			const uint32 instanceRowSize = GetRHIFormatPixelSize(instanceRowFormat);
			vi.Bindings = {
				{0, sizeof(Asset::AssetVertex), false},
				{1, instanceRowSize * 4, true} };
			vi.Attributes = {
				{POSITION(0), 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
				{NORMAL(0), 1, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Normal)},//normal
				{TANGENT(0), 2, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Tangent)},//tangent
				{TEXCOORD(0), 3, 0, ERHIFormat::R32G32_SFLOAT, offsetof(Asset::AssetVertex, UV)},// uv
				// instance transform
				{INSTANCE_TRANSFORM(0), 0, 1, instanceRowFormat, 0},
				{INSTANCE_TRANSFORM(1), 0, 1, instanceRowFormat, instanceRowSize},
				{INSTANCE_TRANSFORM(2), 0, 1, instanceRowFormat, instanceRowSize * 2},
				{INSTANCE_TRANSFORM(3), 0, 1, instanceRowFormat, instanceRowSize * 3}, };
		}
		else {
			vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
			vi.Attributes = {
				{POSITION(0), 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
				{NORMAL(0), 1, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Normal)},//normal
				{TANGENT(0), 2, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Tangent)},//tangent
				{TEXCOORD(0), 3, 0, ERHIFormat::R32G32_SFLOAT, offsetof(Asset::AssetVertex, UV)},// uv
			};
		}
		// others
		desc.BlendDesc.BlendStates = { {false}, {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Back };
		desc.DepthStencilState = { true, true, ECompareType::Less, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
	}

	void MaterialTemplate::BindBasePassShaderPrams(RHICommandBuffer* cmd, bool bInstanced) {
		const uint32 materialSetIdx = GetMaterialSetIndex(bInstanced);
		uint32 bindingIdx = 0;
		// uniform
		if(m_Vectors.Size()) {
			cmd->SetShaderParam(materialSetIdx, bindingIdx++, RHIShaderParam::UniformBuffer(m_MaterialCB));
		}
		for(auto& t: m_Textures) {
			RHITexture* texture = t.Texture ? t.Texture : Render::DefaultResources::Instance()->GetDefaultTexture2D(Render::DefaultResources::TEX_WHITE);
			cmd->SetShaderParam(materialSetIdx, bindingIdx++, RHIShaderParam::Texture(texture));
		}
		for(auto& s : m_Samplers) {
			RHISampler* sampler = Render::DefaultResources::Instance()->GetDefaultSampler(s.Filter, s.AddressMode);
			cmd->SetShaderParam(materialSetIdx, bindingIdx++, RHIShaderParam::Sampler(sampler));
		}
	}

	uint32 MaterialTemplate::GetHash(bool bInstanced) {
		return GetTypeHash32BasedOn(bInstanced, m_Hash);
	}

	void MaterialTemplate::Reload(const Asset::MaterialTemplateAsset& asset) {
		// translate to shader binding
		m_Vectors.Reset();
		m_Textures.Reset();
		m_Samplers.Reset();
		m_MaterialOutputs.Reset();
		// combine vectors and scalars
		const uint32 vectorSize = asset.Vectors.Size();
		const uint32 combinedVectorSize = vectorSize + (asset.Scalars.Size() + 3) / 4;
		m_Vectors.Resize(combinedVectorSize);
		for (uint32 i = 0; i < vectorSize; ++i) {
			m_Vectors[i] = asset.Vectors[i];
		}
		for (uint32 i = 0; i < asset.Scalars.Size(); ++i) {
			const uint32 vectorIndex = vectorSize + i / 4;
			const uint32 vectorOffset = i % 4;
			m_Vectors[vectorIndex][vectorOffset] = asset.Scalars[i];
		}
		// textures
		TSet<uint32> samplersCode;
		m_Textures.Resize(asset.Textures.Size());
		for (uint32 i = 0; i < m_Textures.Size(); ++i) {
			const auto& srcTex = asset.Textures[i];
			m_Textures[i].Texture = StaticResourceMgr::Instance()->GetTexture(srcTex.Texture);
			m_Textures[i].SamplerCode = srcTex.SamplerCode;
			samplersCode.insert(srcTex.SamplerCode);
		}
		// samplers
		m_Samplers.Reserve((uint32)samplersCode.size());
		for (uint32 samplerCode : samplersCode) {
			m_Samplers.PushBack(MaterialSamplerCode{ samplerCode });
		}
		// calculate hash
		m_Hash = 0;
		m_Hash = GetTypeHash32BasedOn(m_Textures.Size(), m_Hash);
		m_Hash = GetTypeHash32BasedOn(m_Vectors.Size(), m_Hash);
		m_Hash = GetTypeHash32BasedOn(m_Samplers.Size(), m_Hash);

		// outputs
		for (const auto& srcParam : asset.Parameters) {
			auto& dstParam = m_MaterialOutputs.EmplaceBack();
			dstParam = srcParam;
			if (dstParam.ParamType == EParameterType::Scalar) {
				dstParam.Offset += 4 * vectorSize;
			}
		}

		// material constant buffer
		if(!m_MaterialCB || m_MaterialCB->GetDesc().ByteSize < m_Vectors.ByteSize()) {
			m_MaterialCB = RHI::Instance()->CreateBuffer({ EBufferFlags::Uniform, m_Vectors.ByteSize(), 0 });
		}
		m_MaterialCB->UpdateData(m_Vectors.Data(), m_Vectors.ByteSize(), 0);
		m_Shader.PS.Reset();
		m_Shader.VS.Reset();
		m_InstancedShader.PS.Reset();
		m_InstancedShader.VS.Reset();
	}

	MaterialTemplate::MaterialShader& MaterialTemplate::GetOrCreateShader(bool bInstanced) {
		auto& shader = bInstanced ? m_InstancedShader : m_Shader;
		if(!shader.VS || !shader.PS) {
			XString shaderSourceCode;
			auto buildShader = [this, &shaderSourceCode, bInstanced](EShaderStageFlags stage, const XString& entry) -> RHIShaderPtr {
				uint32 hash = GetTypeHash32BasedOn(bInstanced, m_Hash);
				const XString compiledFile = ShaderCompiler::GetCompileOutputFile(BASE_PASS_TEMPLATE_SHADER, entry, hash);
				TArray<int8> compiledCode;
				if (!ShaderCompiler::LoadCompiledShader(compiledFile, compiledCode)) {
					if (shaderSourceCode.empty()) {
						GenerateShaderCode(GetMaterialSetIndex(bInstanced), shaderSourceCode);
					}
					TArray<ShaderCompiler::Macro> macros;
					if (bInstanced) {
						macros.PushBack({ "INSTANCED", "" });
					}
					ShaderCompiler::CompileCodeToFile(shaderSourceCode, entry, stage, macros, compiledFile);
					if (!ShaderCompiler::LoadCompiledShader(compiledFile, compiledCode)) {
						LOG_ERROR("Failed to load material shader! %s", compiledFile.c_str());
						return nullptr;
					}
				}
				return RHI::Instance()->CreateShader(stage, compiledCode.Data(), compiledCode.Size(), entry);
			};
			shader.VS = buildShader(EShaderStageFlags::Vertex, "MainVS");
			shader.PS = buildShader(EShaderStageFlags::Pixel, "MainPS");
		}
		return shader;
	}

	void MaterialTemplate::GenerateShaderCode(uint32 materialSet, XString& outCode) {
		// generate shader code
		XString shaderCode;
		CHECK(ShaderCompiler::LoadSourceShader(BASE_PASS_TEMPLATE_SHADER, shaderCode));
		XString layoutDeclareCode, psOutputCode;
		// generate layout declares
		uint32 bindingIdx = 0;
		// vectors
		const uint32 vectorSize = m_Vectors.Size();
		layoutDeclareCode.append(StringFormat("[[vk::binding(%u, %u)]] cbuffer uVector{float4 uVectors[%u];};\n", bindingIdx++, materialSet, vectorSize));
		// textures
		for(uint32 i=0; i<m_Textures.Size(); ++i) {
			layoutDeclareCode.append(StringFormat("[[vk::binding(%u, %u)]] Texture2D uTexture%u;\n", bindingIdx++, materialSet, i));
		}
		// samplers
		for(MaterialSamplerCode samplerCode: m_Samplers) {
			layoutDeclareCode.append(StringFormat("[[vk::binding(%u, %u)]] SamplerState uSampler%u;\n", bindingIdx++, materialSet, samplerCode.Code));
		}
		// generate pbr output defines // TODO more shading models.
		for(auto& param: m_MaterialOutputs) {
			if(INVALID_INDEX == param.Offset) {
				LOG_ERROR("Material output is invalid! %s", param.Name);
				continue;
			}
			// build a float4 value
			XString valueF4;
			switch (param.ParamType) {
			case EParameterType::Vector: {
				valueF4 = StringFormat("uVectors[%u]", param.Offset);
				break;
			}
			case EParameterType::Scalar: {
				uint32 vectorIdx = param.Offset / 4;
				uint32 vectorOffset = param.Offset % 4;
				valueF4 = StringFormat("((float4)uVectors[%u][%u])", vectorIdx, vectorOffset);
				break;
			}
			case EParameterType::Texture: {
				const MaterialSamplerCode samplerCode = m_Textures[param.Offset].SamplerCode;
				valueF4 = StringFormat("uTexture%u.Sample(uSampler%u, pIn.UV)", param.Offset, samplerCode.Code);
				break;
			}
			}
			XString assignmentStr = ConvertAssignmentStr(param.AssignType);
			XString outputStr = ConvertOutputStr(param.OutputType);
			// data.BaseColor = uTexture0.Sample(sampler0, pIn.UV);
			psOutputCode.append(StringFormat("data.%s %s %s.%s;\n", param.Name.c_str(), assignmentStr.c_str(), valueF4.c_str(), outputStr.c_str()));
		}
		outCode = StringFormat<4096>(shaderCode.c_str(), layoutDeclareCode.c_str(), psOutputCode.c_str());
	}

	MaterialInstance::MaterialInstance(const Asset::MaterialAsset& asset, MaterialTemplate* materialTemplate){
		Reload(asset, materialTemplate);
	}

	MaterialInstance::MaterialInstance(MaterialInstance&& rhs) noexcept :
	m_Template(rhs.m_Template),
	m_VectorOverrides(MoveTemp(rhs.m_VectorOverrides)),
	m_TextureOverrides(MoveTemp(rhs.m_TextureOverrides)),
	m_MaterialCB(MoveTemp(rhs.m_MaterialCB)),
	m_CacheHash(rhs.m_CacheHash){}

	void MaterialInstance::FillBasePassPSODesc(RHIGraphicsPipelineStateDesc& desc, bool bInstanced) {
		m_Template->FillBasePassPSODesc(desc, bInstanced);
	}

	void MaterialInstance::BindBasePassShaderPrams(RHICommandBuffer* cmd, bool bInstanced) {
		// check cache hash and fix
		CheckAndFixParameters();
		const uint32 materialSetIdx = GetMaterialSetIndex(bInstanced);
		uint32 bindingIdx = 0;
		// uniform
		if (m_VectorOverrides.Size()) {
			cmd->SetShaderParam(materialSetIdx, bindingIdx++, RHIShaderParam::UniformBuffer(m_MaterialCB));
		}
		for (auto& t : m_TextureOverrides) {
			RHITexture* texture = t ? t : Render::DefaultResources::Instance()->GetDefaultTexture2D(Render::DefaultResources::TEX_WHITE);
			cmd->SetShaderParam(materialSetIdx, bindingIdx++, RHIShaderParam::Texture(texture));
		}
		for (auto& s : m_Template->m_Samplers) {
			RHISampler* sampler = Render::DefaultResources::Instance()->GetDefaultSampler(s.Filter, s.AddressMode);
			cmd->SetShaderParam(materialSetIdx, bindingIdx++, RHIShaderParam::Sampler(sampler));
		}
	}

	uint32 MaterialInstance::GetHash(bool bInstanced) {
		return m_Template->GetHash(bInstanced);
	}

	MaterialTemplate* MaterialInstance::GetTemplate() {
		return m_Template;
	}

	void MaterialInstance::Reload(const Asset::MaterialAsset& asset, MaterialTemplate* materialTemplate) {
		m_Template = materialTemplate;
		// reserve parameters by template
		{
			m_VectorOverrides.Reset();
			m_VectorOverrides = materialTemplate->m_Vectors;
			m_TextureOverrides.Reset();
			m_TextureOverrides.Reserve(materialTemplate->m_Textures.Size());
			for(auto& tex: materialTemplate->m_Textures) {
				m_TextureOverrides.PushBack(tex.Texture);
			}
		}
		// assign new parameters
		{
			const uint32 vectorSize = asset.VectorOverrides.Size();
			const uint32 vectorOverrideSize = Math::Min(vectorSize, m_VectorOverrides.Size());
			for(uint32 i=0; i<vectorOverrideSize; ++i) {
				m_VectorOverrides[i] = asset.VectorOverrides[i];
			}
			const uint32 scalarSize = asset.ScalarOverrides.Size();
			const uint32 scalarOffset = vectorSize * 4;
			const uint32 scalarOverrideMax = Math::Min(scalarOffset + scalarSize, m_VectorOverrides.Size() * 4);
			for(uint32 i=scalarOffset; i< scalarOverrideMax; ++i) {
				((float*)m_VectorOverrides.Data())[i] = asset.ScalarOverrides[i - scalarOffset];
			}
			const uint32 textureSize = asset.TextureOverrides.Size();
			const uint32 textureOverrideSize = Math::Min(textureSize, m_TextureOverrides.Size());
			for(uint32 i=0; i<textureOverrideSize; ++i) {
				m_TextureOverrides[i] = StaticResourceMgr::Instance()->GetTexture(asset.TextureOverrides[i]);
			}
		}
		// material buffer TODO support static buffer allocator.
		m_MaterialCB = RHI::Instance()->CreateBuffer({ EBufferFlags::Uniform, m_VectorOverrides.ByteSize(), 0 });
		m_MaterialCB->UpdateData(m_VectorOverrides.Data(), m_VectorOverrides.ByteSize(), 0);

		m_CacheHash = materialTemplate->m_Hash;
	}

	bool MaterialInstance::CheckAndFixParameters() {
		if (m_CacheHash == m_Template->m_Hash) {
			return true;
		}
		const auto& templateVectors = m_Template->m_Vectors;
		if (const uint32 targetSize = m_VectorOverrides.Size(); targetSize != templateVectors.Size()) {
			m_VectorOverrides.Resize(templateVectors.Size());
			for (uint32 i = targetSize; i < templateVectors.Size(); ++i) {
				m_VectorOverrides[i] = templateVectors[i];
			}
		}
		const auto& templateTextures = m_Template->m_Textures;
		if (const uint32 targetSize = m_TextureOverrides.Size(); targetSize != templateTextures.Size()) {
			m_TextureOverrides.Resize(templateTextures.Size());
			for (uint32 i = targetSize; i < templateTextures.Size(); ++i) {
				m_TextureOverrides[i] = templateTextures[i].Texture;
			}
		}
		return false;
	}

	MaterialTemplate* MaterialMgr::GetMaterialTemplate(const XString& filename) {
		if(auto iter = m_MaterialTemplates.find(filename); iter != m_MaterialTemplates.end()) {
			return &iter->second;
		}

		Asset::MaterialTemplateAsset asset;
		if(!Asset::AssetLoader::LoadProjectAsset(&asset, filename.c_str())) {
			return nullptr;
		}
		return &m_MaterialTemplates.try_emplace(filename, MaterialTemplate(asset)).first->second;
	}

	MaterialInstance* MaterialMgr::GetMaterialInstance(const XString& filename) {
		if(auto iter = m_MaterialInstances.find(filename); iter!= m_MaterialInstances.end()) {
			return &iter->second;
		}
		Asset::MaterialAsset asset;
		if(!Asset::AssetLoader::LoadProjectAsset(&asset, filename.c_str())) {
			return nullptr;
		}
		MaterialTemplate* matTemplate = GetMaterialTemplate(asset.Template);
		if(!matTemplate) {
			return nullptr;
		}
		return &m_MaterialInstances.try_emplace(filename, MaterialInstance(asset, matTemplate)).first->second;
	}

	MaterialInterface* MaterialMgr::GetMaterialInterface(const XString& filename) {
		if(StrEndsWith(filename.c_str(), ".mati")) {
			return GetMaterialInstance(filename);
		}
		else if(StrEndsWith(filename.c_str(), ".matt")) {
			return GetMaterialTemplate(filename);
		}
		return nullptr;
	}

	void MaterialMgr::ReloadMaterialTemplate(const XString& filename, const Asset::MaterialTemplateAsset& asset) {
		if(MaterialTemplate* material = GetMaterialTemplate(filename)) {
			material->Reload(asset);
		}
	}

	void MaterialMgr::ReloadMaterialInstance(const XString& filename, const Asset::MaterialAsset& asset) {
		if(MaterialInstance* material = GetMaterialInstance(filename)) {
			if(MaterialTemplate* materialTemplate = GetMaterialTemplate(asset.Template)) {
				material->Reload(asset, materialTemplate);
			}
		}
	}

	uint32 MaterialContainer::FindOrCreateMaterial(const XString& filename) {
		const uint32 hash = DataArrayHash32(filename.data(), (uint32)filename.size());
		uint32 materialIndex = m_Materials.FindIdx(hash);
		if(INVALID_INDEX == materialIndex) {
			if(MaterialInterface* material = MaterialMgr::Instance()->GetMaterialInterface(filename)) {
				materialIndex = m_Materials.FindOrAddID(hash);
				m_Materials[materialIndex] = material;
			}
		}
		return materialIndex;
	}

	MaterialInterface* MaterialContainer::GetMaterial(uint32 materialIndex) {
		return m_Materials[materialIndex];
	}
}
