#pragma once
#include "RenderResources.h"
#include "Core/Public/SmartPointer.h"


namespace Engine {
	class Camera;
	class Material;
	class DirectionalLight;
	class RenderScene;

	class RenderObject {
		friend RenderScene;
	private:
		RenderScene* m_Scene;
		uint32 m_Index;
		String m_Name{"Unnamed object"};
	public:
		RenderObject(RenderScene* scene);
		virtual ~RenderObject();
		virtual void DrawCall(RHI::RCommandBuffer* cmd, RHI::RPipelineLayout* layout) = 0;
		virtual const char* Name() { return m_Name.c_str(); }
	};

	struct SceneRenderData {
		const Material* Material;
		const BufferCommon* Uniform;
		TVector<Primitive> primitives;
	};

	class RenderScene {
	private:
		friend class RenderSystem;
		TVector<RenderObject*> m_RenderObjects; // all render objects
		TUniquePtr<DirectionalLight> m_DirectionalLight;
		TUniquePtr<Camera> m_Camera;
		RHI::RDescriptorSet* m_SceneDescs;
		BufferCommon m_LightUniform;
		BufferCommon m_CameraUniform;

	private:
		void UpdateUniform();
		void CreateResources();
		void CreateDescriptorSets();
		void Update();
		void RenderGBuffer(RHI::RCommandBuffer* cmd, RHI::RPipelineLayout* layout);
	public:
		static TUniquePtr<RenderScene> s_Default;
		static RenderScene* GetDefaultScene(); // TODO TEST
		static void Clear() { if (s_Default) s_Default.reset(); }
		Camera* GetMainCamera() { return m_Camera.get(); }

		RenderScene();
		~RenderScene();
		const TVector<RenderObject*> GetRenderObjects() const { return m_RenderObjects; }
		void AddRenderObject(RenderObject* obj);
		void RemoveRenderObject(RenderObject* obj);
	};
}
