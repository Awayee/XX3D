#pragma once
#include "RenderCommon.h"
#include "Core/Public/Container.h"
#include "Core/Public/SmartPointer.h"

namespace Engine {
	class Material {
	private:
		RHI::RDescriptorSet* m_DescriptorSet{nullptr};
	public:
		Material();
		~Material();
		void SetTexture(TextureCommon* tex);
		RHI::RDescriptorSet* GetDescs() { return m_DescriptorSet; }
	};

	class MaterialMgr: public TSingleton<MaterialMgr> {
	private:
		friend TSingleton<MaterialMgr>;
		TStrMap<Material> m_MaterialMap;
		TUniquePtr<Material> m_DefaultMaterial;
		MaterialMgr() = default;
		~MaterialMgr() {};
		Material* GetMaterial(const char* file);
		Material* GetDefaultMaterial();
	public:
		static Material* Get(const char* file) { return Instance()->GetMaterial(file); }
		static Material* GetDefault() { return Instance()->GetDefaultMaterial(); }
	};
}
