#pragma once
#include "Render/Public/RenderCommon.h"
#include "Core/Public/TVector.h"
#include "Core/Public/TPtr.h"

namespace Engine {
	class Material {
	private:
		Engine::RDescriptorSet* m_DescriptorSet{nullptr};
	public:
		Material();
		~Material();
		void SetTexture(TextureCommon* tex);
		Engine::RDescriptorSet* GetDescs() { return m_DescriptorSet; }
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
