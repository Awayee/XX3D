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
enum EMaterialShaderParamSet : uint32{
	MATERIAL_PARAM_SET_SCENE = 0,
	MATERIAL_PARAM_SET_OBJECT = 1,
	MATERIAL_PARAM_SET_MATERIAL = 2,
	MATERIAL_PARAM_SET_COUNT
};

namespace Object {
	static const ShaderCompiler::Macro sInstancedMacro{ "INSTANCED", "1" };
	static const char* MainVS = "MainVS";
	static const char* MainPS = "MainPS";

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

	inline TConstArrayView<ShaderCompiler::Macro> GetMaterialShaderMacro(bool bInstanced) {
		if(bInstanced) {
			return TConstArrayView<ShaderCompiler::Macro>(&sInstancedMacro, 1);
		}
		return TConstArrayView<ShaderCompiler::Macro>();
	}

	RHIShaderBinding MaterialShader::uCamera{0, 0, EBindingType::UniformBuffer, 1};
	RHIShaderBinding MaterialShader::uInstanceData{0, 1, EBindingType::StructuredBuffer, 1};
	RHIShaderBinding MaterialShader::uInstanceID{1, 1, EBindingType::StructuredBuffer, 1};
	RHIShaderBinding MaterialShader::uModel{0, 1, EBindingType::UniformBuffer, 1};

	MaterialShader::MaterialShader(EShaderStageFlags stage, XStringView code, XStringView entryName) {
		m_RHIShader = RHI::Instance()->CreateShader(stage, code, entryName, this);
	}

	RHIShader* MaterialShader::GetRHI() {
		return m_RHIShader.Get();
	}

	template<bool bInstanced> class MaterialShaderVS : public MaterialShader{
	public:
		MaterialShaderVS(XStringView code, XStringView entryName): MaterialShader(EShaderStageFlags::Vertex, code, entryName) {}
		void GetBindings(RHIShaderBindingSet& bindingSet) override {
			const EShaderStageFlags stage = m_RHIShader->GetStage();
			bindingSet.AddBinding(uCamera, stage);
			if constexpr(bInstanced) {
				bindingSet.AddBinding(uInstanceData, stage);
				bindingSet.AddBinding(uInstanceID, stage);
			}
			else {
				bindingSet.AddBinding(uModel, stage);
			}
		}
	};

	class MaterialShaderPS: public MaterialShader {
	public:
		MaterialShaderPS(XStringView code, XStringView entryName, MaterialDataLayout layout):
		MaterialShader(EShaderStageFlags::Pixel, code, entryName),
		m_Layout(layout){}

		void GetBindings(RHIShaderBindingSet& bindingSet) override {
			const EShaderStageFlags stage = m_RHIShader->GetStage();
			uint32 binding = 0;
			for(uint32 i=0; i<m_Layout.GetNumUniformBuffers(); ++i) {
				bindingSet.AddBinding(RHIShaderBinding{ binding++, MATERIAL_PARAM_SET_MATERIAL, EBindingType::UniformBuffer, 1 }, stage);
			}
			for(uint32 i=0; i<m_Layout.NumTextures; ++i) {
				bindingSet.AddBinding(RHIShaderBinding{ binding++, MATERIAL_PARAM_SET_MATERIAL, EBindingType::Texture, 1 }, stage);
			}
			for(uint32 i=0; i<m_Layout.NumSamplers; ++i) {
				bindingSet.AddBinding(RHIShaderBinding{ binding++, MATERIAL_PARAM_SET_MATERIAL, EBindingType::Sampler, 1 }, stage);
			}
		}
	private:
		MaterialDataLayout m_Layout;
	};

	bool MaterialDataLayout::operator==(const MaterialDataLayout& rhs) const {
		return (*reinterpret_cast<const uint32*>(this)) == (*reinterpret_cast<const uint32*>(&rhs));
	}

	uint32 MaterialDataLayout::GetNumMergedVectors() const {
		return NumVectors + (NumScalars + 3) / 4;
	}

	uint32 MaterialDataLayout::GetNumUniformBuffers() const {
		return (NumVectors + NumScalars > 0) ? 1 : 0;
	}

	uint32 MaterialDataLayout::GetNumBindings() const {
		return GetNumUniformBuffers() + NumVectors + NumScalars;
	}

	uint32 MaterialDataLayout::GetBindingIndexOfTexture(uint32 i) const {
		return GetNumUniformBuffers() + i;
	}

	uint32 MaterialDataLayout::GetBindingIndexOfSampler(uint32 i) const {
		return GetNumUniformBuffers() + NumTextures + i;
	}

	MaterialTemplate::MaterialTemplate(const Asset::MaterialTemplateAsset& asset) : m_Hash(0) {
		Reload(asset);
	}

	MaterialTemplate::MaterialTemplate(MaterialTemplate&& rhs) noexcept :
	m_Layout(rhs.m_Layout),
	m_BindingDatas(MoveTemp(rhs.m_BindingDatas)),
	m_MaterialOutputs(MoveTemp(rhs.m_MaterialOutputs)),
	m_Shader(MoveTemp(rhs.m_Shader)),
	m_InstancedShader(MoveTemp(rhs.m_InstancedShader)),
	m_MaterialCB(MoveTemp(rhs.m_MaterialCB)),
	m_Hash(rhs.m_Hash){}

	void MaterialTemplate::FillBasePassPSODesc(RHIGraphicsPipelineStateDesc& desc, bool bInstanced) {
		// shader
		auto& shader = GetOrCreateShader(bInstanced);
		desc.VertexShader = shader.VS->GetRHI();
		desc.PixelShader = shader.PS->GetRHI();
		// vertex input;
		auto& vi = desc.VertexInput;
		vi.Bindings = { {0, sizeof(Asset::AssetVertex), false} };
		vi.Attributes = {
			{POSITION(0), 0, 0, ERHIFormat::R32G32B32_SFLOAT, 0},// position
			{NORMAL(0), 1, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Normal)},//normal
			{TANGENT(0), 2, 0, ERHIFormat::R32G32B32_SFLOAT, offsetof(Asset::AssetVertex, Tangent)},//tangent
			{TEXCOORD(0), 3, 0, ERHIFormat::R32G32_SFLOAT, offsetof(Asset::AssetVertex, UV)},// uv
		};
		// others
		desc.BlendDesc.BlendStates = { {false}, {false} };
		desc.RasterizerState = { ERasterizerFill::Solid, ERasterizerCull::Back };
		desc.DepthStencilState = { true, true, ECompareType::Less, false };
		desc.PrimitiveTopology = EPrimitiveTopology::TriangleList;
	}

	void MaterialTemplate::BindBasePassShaderPrams(RHICommandBuffer* cmd, bool bInstanced) {
		// uniform
		if(m_Layout.GetNumUniformBuffers()) {
			const BindingData& data = m_BindingDatas[0];
			cmd->SetShaderParam(RHIShaderBinding{ 0, MATERIAL_PARAM_SET_MATERIAL, EBindingType::UniformBuffer, 1 },
				RHIShaderParam::UniformBuffer(m_MaterialCB, data.BufferOffset, data.BufferSize));
		}
		for(uint32 i=0; i<m_Layout.NumTextures; ++i) {
			const uint32 binding = m_Layout.GetBindingIndexOfTexture(i);
			RHITexture* t = m_BindingDatas[binding].Texture;
			t = t ? t : Render::DefaultResources::Instance()->GetDefaultTexture2D(Render::DefaultResources::TEX_WHITE);
			cmd->SetShaderParam(RHIShaderBinding{ binding, MATERIAL_PARAM_SET_MATERIAL, EBindingType::Texture, 1 }, RHIShaderParam::Texture(t));
		}
		for(uint32 i=0; i<m_Layout.NumSamplers; ++i) {
			const uint32 binding = m_Layout.GetBindingIndexOfSampler(i);
			const MaterialSamplerCode sCode{ m_BindingDatas[binding].Sampler };
			RHISampler* s = Render::DefaultResources::Instance()->GetDefaultSampler(sCode.Filter, sCode.AddressMode);
			cmd->SetShaderParam(RHIShaderBinding{ binding, MATERIAL_PARAM_SET_MATERIAL, EBindingType::Sampler, 1 }, RHIShaderParam::Sampler(s));
		}
	}

	uint32 MaterialTemplate::GetHash(bool bInstanced) {
		return GetTypeHash32BasedOn(bInstanced, m_Hash);
	}

	uint32 MaterialTemplate::GetTemplateHash() const {
		return m_Hash;
	}

	void MaterialTemplate::Reload(const Asset::MaterialTemplateAsset& asset) {
		// translate to shader binding
		MaterialDataLayout layout;
		layout.NumVectors = asset.Vectors.Size();
		layout.NumScalars = asset.Scalars.Size();
		layout.NumTextures = asset.Textures.Size();
		m_MaterialOutputs.Reset();
		m_BindingDatas.Reset();
		// combine vectors and scalars
		const uint32 numMergedVectors = layout.GetNumMergedVectors();
		if(numMergedVectors) {
			TFixedArray<Math::FVector4> vectors(numMergedVectors);
			for(uint32 i=0; i<layout.NumVectors; ++i) {
				vectors[i] = asset.Vectors[i];
			}
			for(uint32 i=0; i<layout.NumScalars; ++i) {
				const uint32 vectorIndex = layout.NumVectors + i / 4;
				const uint32 vectorOffset = i % 4;
				vectors[vectorIndex][vectorOffset] = asset.Scalars[i];
			}

			// material constant buffer
			const uint32 bufferSize = numMergedVectors * sizeof(Math::FVector4);
			if (!m_MaterialCB || m_MaterialCB->GetDesc().ByteSize < bufferSize) {
				m_MaterialCB = RHI::Instance()->CreateBuffer({ EBufferFlags::Uniform, bufferSize, 0 });
			}
			m_MaterialCB->UpdateData(vectors.Data(), bufferSize, 0);
			BindingData& data = m_BindingDatas.EmplaceBack();
			data.Buffer = m_MaterialCB.Get();
			data.BufferOffset = 0;
			data.BufferSize = bufferSize;
		}
		// textures
		TSet<uint32> samplersCode;
		for (uint32 i=0; i<layout.NumTextures; ++i) {
			const Asset::MaterialTemplateAsset::TextureParameter& srcTex = asset.Textures[i];
			BindingData& data = m_BindingDatas.EmplaceBack();
			data.Texture = StaticResourceMgr::Instance()->GetTexture(srcTex.Texture);
			data.TexSampleCode = srcTex.SamplerCode;
			samplersCode.insert(srcTex.SamplerCode);
		}
		// samplers
		layout.NumSamplers = samplersCode.size();
		for (const uint32 samplerCode : samplersCode) {
			BindingData& data = m_BindingDatas.EmplaceBack();
			data.Sampler = samplerCode;
		}

		// outputs
		for (const auto& srcParam : asset.Parameters) {
			auto& dstParam = m_MaterialOutputs.EmplaceBack();
			dstParam = srcParam;
			if (dstParam.ParamType == EParameterType::Scalar) {
				dstParam.Offset += 4 * layout.NumVectors;
			}
		}

		// calculate hash
		uint32 hash = 0;
		hash = GetTypeHash32BasedOn(numMergedVectors, hash);
		hash = GetTypeHash32BasedOn(layout.NumTextures, hash);
		hash = GetTypeHash32BasedOn(layout.NumSamplers, hash);
		// need rebuild shader if hash modified
		if(hash != m_Hash) {
			m_Shader.PS.Reset();
			m_Shader.VS.Reset();
			m_InstancedShader.PS.Reset();
			m_InstancedShader.VS.Reset();
			m_Hash = hash;
		}
		m_Layout = layout;
	}

	MaterialDataLayout MaterialTemplate::GetLayout() const {
		return m_Layout;
	}

	TConstArrayView<MaterialTemplate::BindingData> MaterialTemplate::GetBindingDatas() {
		return m_BindingDatas;
	}

	const MaterialTemplate::BindingData& MaterialTemplate::GetBindingData(uint32 binding) {
		CHECK(binding < m_BindingDatas.Size());
		return m_BindingDatas[binding];
	}

	MaterialTemplate::MaterialShaderSet& MaterialTemplate::GetOrCreateShader(bool bInstanced) {
		auto& shader = bInstanced ? m_InstancedShader : m_Shader;
		XString shaderSourceCode;
		TArray<int8> compiledCode;
		auto buildShaderCode = [this, &shaderSourceCode, &compiledCode, bInstanced](EShaderStageFlags stage, const XStringView entry)->XStringView {
			uint32 hash = GetTypeHash32BasedOn(bInstanced, m_Hash);
			XString compiledFile = ShaderCompiler::GetCompileOutputFile(BASE_PASS_TEMPLATE_SHADER, entry, hash);
			if (!ShaderCompiler::LoadCompiledShader(compiledFile, compiledCode)) {
				if (shaderSourceCode.empty()) {
					GenerateShaderCode(shaderSourceCode);
				}
				if(!ShaderCompiler::CompileCodeToBytes(shaderSourceCode, entry, stage, GetMaterialShaderMacro(bInstanced), compiledCode)) {
					LOG_ERROR("Failed to load material shader! %s", compiledFile.c_str());
					return {};
				}
				ShaderCompiler::SaveCompiledBytesToFile(compiledCode, compiledFile);
				LOG_DEBUG("Material shader compiled %s %s", XString{ entry }.c_str(), compiledFile.c_str());
			}
			return XStringView{compiledCode.Data(), compiledCode.Size()};
		};
		if(!shader.VS) {
			if (XStringView code=buildShaderCode(EShaderStageFlags::Vertex, MainVS); !code.empty()) {
				if(bInstanced) {
					shader.VS.Reset(new MaterialShaderVS<true>(code, MainVS));
				}
				else {
					shader.VS.Reset(new MaterialShaderVS<false>(code, MainVS));
				}
			}
		}
		if(!shader.PS) {
			if(XStringView code = buildShaderCode(EShaderStageFlags::Pixel, MainPS); !code.empty()) {
				shader.PS.Reset(new MaterialShaderPS(code, MainPS, m_Layout));
			}
		}
		return shader;
	}

	void MaterialTemplate::GenerateShaderCode(XString& outCode) {
		// generate shader code
		XString shaderCode;
		CHECK(ShaderCompiler::LoadSourceShader(BASE_PASS_TEMPLATE_SHADER, shaderCode));
		XString layoutDeclareCode, psOutputCode;
		const MaterialDataLayout layout = m_Layout;
		// generate layout declares
		if(uint32 numMergedVectors = layout.GetNumMergedVectors()) {
			layoutDeclareCode.append(StringFormat("[[vk::binding(%u, %u)]] cbuffer uVector{float4 uVectors[%u];};\n", 0, MATERIAL_PARAM_SET_MATERIAL, numMergedVectors));
		}
		for(uint32 i=0; i< layout.NumTextures; ++i) {
			const uint32 binding = layout.GetBindingIndexOfTexture(i);
			layoutDeclareCode.append(StringFormat("[[vk::binding(%u, %u)]] Texture2D uTexture%u;\n", binding, MATERIAL_PARAM_SET_MATERIAL, i));
		}
		for(uint32 i=0; i< layout.NumSamplers; ++i) {
			const uint32 binding = layout.GetBindingIndexOfSampler(i);
			uint32 samplerCode = m_BindingDatas[binding].Sampler;
			layoutDeclareCode.append(StringFormat("[[vk::binding(%u, %u)]] SamplerState uSampler%u;\n", binding, MATERIAL_PARAM_SET_MATERIAL, samplerCode));
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
				const uint32 binding = layout.GetBindingIndexOfSampler(param.Offset);
				const uint32 samplerCode = m_BindingDatas[binding].Sampler;
				valueF4 = StringFormat("uTexture%u.Sample(uSampler%u, pIn.UV)", param.Offset, samplerCode);
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
	m_Overrides(rhs.m_Overrides),
	m_MaterialCB(MoveTemp(rhs.m_MaterialCB)),
	m_CacheLayout(rhs.m_CacheLayout){}

	void MaterialInstance::FillBasePassPSODesc(RHIGraphicsPipelineStateDesc& desc, bool bInstanced) {
		m_Template->FillBasePassPSODesc(desc, bInstanced);
	}

	void MaterialInstance::BindBasePassShaderPrams(RHICommandBuffer* cmd, bool bInstanced) {
		// check cache hash and fix
		CheckAndFixParameters();
		const MaterialDataLayout layout = m_Template->GetLayout();
		if(layout.GetNumUniformBuffers()) {
			RHIShaderParam param;
			if(m_MaterialCB) {
				const OverrideData& data = m_Overrides[0];
				param = RHIShaderParam::UniformBuffer(m_MaterialCB.Get(), data.BufferOffset, data.BufferSize);
			}
			else {
				const MaterialTemplate::BindingData& templateData = m_Template->GetBindingData(0);
				param = RHIShaderParam::UniformBuffer(templateData.Buffer, templateData.BufferOffset, templateData.BufferSize);
			}
			cmd->SetShaderParam(RHIShaderBinding{ 0, MATERIAL_PARAM_SET_MATERIAL, EBindingType::UniformBuffer, 1 }, param);
		}
		for(uint32 i=0; i<layout.NumTextures; ++i) {
			const uint32 binding = layout.GetBindingIndexOfTexture(i);
			RHITexture* t = m_Overrides[binding].Texture;
			t = t ? t : Render::DefaultResources::Instance()->GetDefaultTexture2D(Render::DefaultResources::TEX_WHITE);
			cmd->SetShaderParam(RHIShaderBinding{ binding, MATERIAL_PARAM_SET_MATERIAL, EBindingType::Texture, 1 },
				RHIShaderParam::Texture(t));
		}
		for(uint32 i=0; i<layout.NumSamplers; ++i) {
			const uint32 binding = layout.GetBindingIndexOfSampler(i);
			MaterialSamplerCode sCode{ m_Template->GetBindingData(binding).Sampler };
			RHISampler* s = Render::DefaultResources::Instance()->GetDefaultSampler(sCode.Filter, sCode.AddressMode);
			cmd->SetShaderParam(RHIShaderBinding{ binding, MATERIAL_PARAM_SET_MATERIAL, EBindingType::Sampler, 1 },
				RHIShaderParam::Sampler(s));
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
		const MaterialDataLayout layout = materialTemplate->GetLayout();
		m_Overrides.Reset();
		m_Overrides.Reserve(layout.GetNumBindings());
		// uniform buffer
		if (const uint32 numMergedVectors = layout.GetNumMergedVectors()) {
			TFixedArray<Math::FVector4> vectors(numMergedVectors);
			for(uint32 i=0; i<layout.NumVectors; ++i) {
				if(i < asset.VectorOverrides.Size()) {
					vectors[i] = asset.VectorOverrides[i];
				}
			}
			for(uint32 i=0; i<layout.NumScalars; ++i) {
				if(i<asset.ScalarOverrides.Size()) {
					((float*)vectors.Data())[i + layout.NumVectors * 4] = asset.ScalarOverrides[i];
				}
			}
			// material buffer TODO support static buffer allocator.
			const uint32 bufferSize = numMergedVectors * sizeof(Math::FVector4);
			if(!m_MaterialCB || m_MaterialCB->GetDesc().ByteSize < bufferSize) {
				m_MaterialCB = RHI::Instance()->CreateBuffer({ EBufferFlags::Uniform, bufferSize, 0 });
			}
			m_MaterialCB->UpdateData(vectors.Data(), bufferSize, 0);
			OverrideData& data = m_Overrides.EmplaceBack();
			data.BufferOffset = 0;
			data.BufferSize = bufferSize;
		}
		// textures
		for(uint32 i=0; i<layout.NumTextures; ++i) {
			OverrideData& data = m_Overrides.EmplaceBack();
			data.Texture = i<asset.TextureOverrides.Size() ? StaticResourceMgr::Instance()->GetTexture(asset.TextureOverrides[i]) : nullptr;
		}
		m_CacheLayout = layout;
	}

	bool MaterialInstance::CheckAndFixParameters() {
		const MaterialDataLayout layout = m_Template->GetLayout();
		if(m_CacheLayout == layout) {
			return true;
		}
		// buffer
		TArray<OverrideData> newOverrides; newOverrides.Reserve(layout.GetNumBindings());
		if(layout.NumTextures != m_CacheLayout.NumTextures || layout.NumScalars != m_CacheLayout.NumScalars) {
			OverrideData& data = newOverrides.EmplaceBack();
			data.BufferOffset = 0;
			data.BufferSize = layout.GetNumMergedVectors() * sizeof(Math::FVector4);
			m_MaterialCB.Reset();
		}
		// textures
		if(layout.NumTextures != m_CacheLayout.NumTextures) {
			for (uint32 i = 0; i < layout.NumTextures; ++i) {
				OverrideData& data = newOverrides.EmplaceBack();
				const MaterialTemplate::BindingData& srcData = m_Template->GetBindingData(layout.GetBindingIndexOfTexture(i));
				data.Texture = srcData.Texture;
			}
		}
		else {
			for(uint32 i=0; i<layout.NumTextures; ++i) {
				newOverrides.PushBack(m_Overrides[m_CacheLayout.GetBindingIndexOfTexture(i)]);
			}
		}
		m_Overrides.Swap(newOverrides);
		m_CacheLayout = layout;
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
		LOG_ERROR("Failed to load material: %s", filename.c_str());
		return GetDefaultMaterial();
	}

	MaterialInterface* MaterialMgr::GetDefaultMaterial() {
		return m_DefaultMaterial.Get();
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

	MaterialMgr::MaterialMgr() {
		Asset::MaterialTemplateAsset asset;
		Asset::MaterialTemplateAsset::InitDefault(asset);
		m_DefaultMaterial.Reset(new MaterialTemplate(asset));
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
