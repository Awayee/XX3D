#pragma once
#include "Asset/Public/MaterialAsset.h"
#include "RHI/Public/RHI.h"
#include "Core/Public/Container.h"

namespace Object {

	union MaterialSamplerCode {
		uint32 Code;
		struct {
			ESamplerFilter Filter;
			ESamplerAddressMode AddressMode;
		};
		MaterialSamplerCode() : Code(0) {}
		MaterialSamplerCode(uint32 code) : Code(code) {}
		MaterialSamplerCode(ESamplerFilter filter, ESamplerAddressMode addressMode): Filter(filter), AddressMode(addressMode){}
	};

	class MaterialInterface {
	public:
		virtual ~MaterialInterface() = default;
		virtual void FillBasePassPSODesc(RHIGraphicsPipelineStateDesc& desc, bool bInstanced) {/*Do nothing*/ }
		virtual void FillShadowPassPSODesc(RHIGraphicsPipelineStateDesc& desc, bool bInstanced) {/*Do nothing*/ }
		virtual void BindBasePassShaderPrams(RHICommandBuffer* cmd, bool bInstanced) {/*Do nothing*/ }
		virtual void BindShadowPassShaderParams(RHICommandBuffer* cmd, bool bInstanced) {/*Do nothing*/ }
		virtual uint32 GetHash(bool bInstanced) = 0;
	};

	// A material template contains parameter layout of sub materials
	class MaterialTemplate : public MaterialInterface{
	public:
		NON_COPYABLE(MaterialTemplate);
		MaterialTemplate(const Asset::MaterialTemplateAsset& asset);
		MaterialTemplate(MaterialTemplate&& rhs)noexcept;
		void FillBasePassPSODesc(RHIGraphicsPipelineStateDesc& desc, bool bInstanced) override;
		void BindBasePassShaderPrams(RHICommandBuffer* cmd, bool bInstanced) override;
		uint32 GetHash(bool bInstanced) override;
		void Reload(const Asset::MaterialTemplateAsset& asset);

	private:
		friend class MaterialInstance;
		// default values
		TArray<Math::FVector4> m_Vectors;
		struct BindingTexture {
			RHITexture* Texture;
			MaterialSamplerCode SamplerCode;
		};
		TArray<BindingTexture> m_Textures;
		TArray<MaterialSamplerCode> m_Samplers;
		TArray<Asset::MaterialTemplateAsset::ParameterDesc> m_MaterialOutputs; // for generating code
		// prepare shader for building pso
		struct MaterialShader {
			RHIShaderPtr VS;
			RHIShaderPtr PS;
		};
		MaterialShader m_Shader;
		MaterialShader m_InstancedShader;
		// material constant buffer TODO static buffer memory allocator
		RHIBufferPtr m_MaterialCB;
		uint32 m_Hash;
		MaterialShader& GetOrCreateShader(bool bInstanced);
		void GenerateShaderCode(uint32 materialSet, XString& outCode);
	};

	// Material instances are sub objects of material templates, only contains values.
	class MaterialInstance : public MaterialInterface {
	public:
		NON_COPYABLE(MaterialInstance);
		MaterialInstance(const Asset::MaterialAsset& asset, MaterialTemplate* materialTemplate);
		MaterialInstance(MaterialInstance&& rhs) noexcept;
		void FillBasePassPSODesc(RHIGraphicsPipelineStateDesc& desc, bool bInstanced) override;
		void BindBasePassShaderPrams(RHICommandBuffer* cmd, bool bInstanced) override;
		uint32 GetHash(bool bInstanced) override;
		MaterialTemplate* GetTemplate();
		void Reload(const Asset::MaterialAsset& asset, MaterialTemplate* materialTemplate);
	private:
		MaterialTemplate* m_Template;
		TArray<Math::FVector4> m_VectorOverrides;
		TArray<RHITexture*> m_TextureOverrides;
		RHIBufferPtr m_MaterialCB;
		uint32 m_CacheHash;
		bool CheckAndFixParameters();
	};

	class MaterialHandle {
	private:
		friend class MaterailMgr;
		uint32 MaterialIndex : 31;
		uint32 IsTemplate : 1;
	};

	class MaterialMgr {
		SINGLETON_INSTANCE(MaterialMgr);
	public:
		MaterialTemplate* GetMaterialTemplate(const XString& filename);
		MaterialInstance* GetMaterialInstance(const XString& filename);
		MaterialInterface* GetMaterialInterface(const XString& filename);
		// EDITOR reload interface
		void ReloadMaterialTemplate(const XString& filename, const Asset::MaterialTemplateAsset& asset);
		void ReloadMaterialInstance(const XString& filename, const Asset::MaterialAsset& asset);
	private:
		TMap<XString, MaterialTemplate> m_MaterialTemplates;
		TMap<XString, MaterialInstance> m_MaterialInstances;
		MaterialMgr() = default;
		~MaterialMgr() = default;
	};

	// a array of materials in a scene or render pass.
	class MaterialContainer {
	public:
		uint32 FindOrCreateMaterial(const XString& filename);// create material by .mati or .matt file, return material id
		MaterialInterface* GetMaterial(uint32 materialIndex);
	private:
		TIDMapContainer<MaterialInterface*> m_Materials;
	};
}