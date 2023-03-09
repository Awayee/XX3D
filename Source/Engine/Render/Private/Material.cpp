#include "Render/Public/Material.h"
#include "Render/Public/RenderResources.h"

namespace Engine {
	Material::Material() {
		m_DescriptorSet = RHI_INSTANCE->AllocateDescriptorSet(DescsMgr::Get(DESCS_MATERIAL));
	}

	Material::~Material(){
		//RHI_INSTANCE->FreeDescriptorSet(m_DescriptorSet);
		//m_DescriptorSet = nullptr;
	}

	void Material::SetTexture(TextureCommon* tex) {
		if (nullptr != tex) {
			m_DescriptorSet->UpdateImageSampler(0, SamplerMgr::Get(SAMPLER_DEFAULT), tex->View);
		}
	}

	Material* MaterialMgr::GetMaterial(const char* file) {
		auto finded = m_MaterialMap.find(file);
		if(finded != m_MaterialMap.end()) {
			return &finded->second;
		}
		Material& mat = m_MaterialMap.insert({ file, {} }).first->second;
		mat.SetTexture(TextureMgr::Get(file));
		return &mat;
	}

	Material* MaterialMgr::GetDefaultMaterial(){
		if(m_DefaultMaterial) {
			return m_DefaultMaterial.get();
		}
		m_DefaultMaterial.reset(new Material());
		m_DefaultMaterial->SetTexture(TextureMgr::Get(TextureMgr::WHITE));
		return m_DefaultMaterial.get();
	}
}
