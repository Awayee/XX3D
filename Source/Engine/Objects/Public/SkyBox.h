#pragma once
#include "RHI/Public/RHI.h"
#include "Render/Public/DrawCall.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/Level.h"
#include "Objects/Public/RenderResource.h"

namespace Object {
	struct SkyBoxECSComp {
		PrimitiveResource* Primitive;
		RHITexture* CubeMap;
		SkyBoxECSComp();
		void LoadCubeMap(const XString& file);
		REGISTER_ECS_COMPONENT(SkyBoxECSComp);
	};

	class SkyBoxComponent: public ActorComponent {
	public:
		void OnLoad(const Json::Value& val) override;
		void OnAdd() override;
		void OnRemove() override;
		void SetCubeMapFile(const XString& file);
		const XString& GetCubeMapFile() const { return m_CubeMapFile; }
	private:
		XString m_CubeMapFile;
		REGISTER_ACTOR_COMPONENT(SkyBoxComponent);
	};

	class SkyBoxSystem : public ECSSystem<SkyBoxECSComp> {
		void Update(ECSScene* scene, SkyBoxECSComp* component) override;
		RENDER_SCENE_REGISTER_SYSTEM(SkyBoxSystem);
	};
}