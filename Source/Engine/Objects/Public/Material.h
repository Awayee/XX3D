#pragma once
#include "Asset/Public/MaterialAsset.h"
#include "RHI/Public/RHI.h"
#include "Core/Public/Container.h"

namespace Object {

	class MaterialShader: public RHIShaderBindingInterface {
	public:
		static RHIShaderBinding uCamera;
		static RHIShaderBinding uInstanceData;
		static RHIShaderBinding uInstanceID;
		static RHIShaderBinding uModel;
		MaterialShader(EShaderStageFlags stage, XStringView code, XStringView entryName);
		RHIShader* GetRHI();
	protected:
		RHIShaderPtr m_RHIShader;
	};

	union MaterialSamplerCode {
		uint32 Code;
		struct {
			ESamplerFilter Filter;
			ESamplerAddressMode AddressMode;
		};
		MaterialSamplerCode(uint32 code) : Code(code) {}
	};

	struct MaterialDataLayout {
		uint32 NumVectors : 8;
		uint32 NumScalars : 8;
		uint32 NumTextures: 8;
		uint32 NumSamplers: 8;
		bool operator==(const MaterialDataLayout& rhs)const;
		uint32 GetNumMergedVectors() const;
		uint32 GetNumUniformBuffers() const;
		uint32 GetNumBindings() const;
		uint32 GetBindingIndexOfTexture(uint32 i)const;
		uint32 GetBindingIndexOfSampler(uint32 i)const;
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
		union BindingData {
			struct {
				RHIBuffer* Buffer;
				uint32 BufferOffset;
				uint32 BufferSize;
			};
			struct {
				RHITexture* Texture;
				uint32 TexSampleCode;
			};
			uint32 Sampler;
		};
		NON_COPYABLE(MaterialTemplate);
		MaterialTemplate(const Asset::MaterialTemplateAsset& asset);
		MaterialTemplate(MaterialTemplate&& rhs)noexcept;
		void FillBasePassPSODesc(RHIGraphicsPipelineStateDesc& desc, bool bInstanced) override;
		void BindBasePassShaderPrams(RHICommandBuffer* cmd, bool bInstanced) override;
		uint32 GetHash(bool bInstanced) override;
		uint32 GetTemplateHash() const;
		void Reload(const Asset::MaterialTemplateAsset& asset);
		MaterialDataLayout GetLayout() const;
		TConstArrayView<MaterialTemplate::BindingData> GetBindingDatas();
		const MaterialTemplate::BindingData& GetBindingData(uint32 binding);
	private:
		friend class MaterialInstance;
		// default values
		uint32 m_Hash;
		MaterialDataLayout m_Layout;
		TArray<BindingData> m_BindingDatas;
		TArray<Asset::MaterialTemplateAsset::ParameterDesc> m_MaterialOutputs; // for generating code
		// prepare shader for building pso
		struct MaterialShaderSet {
			TUniquePtr<MaterialShader> VS;
			TUniquePtr<MaterialShader> PS;
		};
		MaterialShaderSet m_Shader;
		MaterialShaderSet m_InstancedShader;
		// material constant buffer TODO static buffer memory allocator
		RHIBufferPtr m_MaterialCB;
		MaterialShaderSet& GetOrCreateShader(bool bInstanced);
		void GenerateShaderCode(XString& outCode);
	};

	// Material instances are sub objects of material templates, only contains values.
	class MaterialInstance : public MaterialInterface {
	public:
		union OverrideData {
			struct {
				uint32 BufferOffset;
				uint32 BufferSize;
			};
			RHITexture* Texture;
		};
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
		TArray<OverrideData> m_Overrides;
		RHIBufferPtr m_MaterialCB;
		MaterialDataLayout m_CacheLayout;
		bool CheckAndFixParameters();
	};

	class MaterialHandle {
	private:
		friend class MaterialMgr;
		uint32 MaterialIndex : 31;
		uint32 IsTemplate : 1;
	};

	class MaterialMgr {
		SINGLETON_INSTANCE(MaterialMgr);
	public:
		MaterialTemplate* GetMaterialTemplate(const XString& filename);
		MaterialInstance* GetMaterialInstance(const XString& filename);
		MaterialInterface* GetMaterialInterface(const XString& filename);
		MaterialInterface* GetDefaultMaterial();
		// EDITOR reload interface
		void ReloadMaterialTemplate(const XString& filename, const Asset::MaterialTemplateAsset& asset);
		void ReloadMaterialInstance(const XString& filename, const Asset::MaterialAsset& asset);
	private:
		TMap<XString, MaterialTemplate> m_MaterialTemplates;
		TMap<XString, MaterialInstance> m_MaterialInstances;
		TUniquePtr<MaterialInterface> m_DefaultMaterial;
		MaterialMgr();
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