#pragma once
#include "Asset/Public/LevelAsset.h"
#include "Core/Public/TUniquePtr.h"
#include "Core/Public/TSingleton.h"
#include "RHI/Public/RHI.h"
#include "Render/Public/DrawCall.h"

namespace Object {
	class Camera;
	class DirectionalLight;
	class RenderScene;

	class RenderObject {
		friend RenderScene;
		static constexpr uint32 INVALID_INDEX = UINT32_MAX;
	protected:
		RenderScene* m_Scene;
		uint32 m_Index;
	public:
		virtual void Update() = 0;
		RenderObject(RenderScene* scene);
		virtual ~RenderObject();
	};

	class SceneRenderSystemBase {
	public:
		virtual void GenerateDrawCall(Render::DrawCallQueue& queue) = 0;
		virtual void Update() = 0;
	};

	class RenderScene {
	public:
		NON_COPYABLE(RenderScene);
		RenderScene();
		~RenderScene();
		RenderScene(RenderScene&& rhs)noexcept;
		RenderScene& operator=(RenderScene&& rhs)noexcept;
		const TVector<RenderObject*> GetRenderObjects() const { return m_RenderObjects; }
		Camera* GetMainCamera() { return m_Camera.Get(); }
		void AddRenderObject(RenderObject* obj);
		void RemoveRenderObject(RenderObject* obj);
		void Update();
		void GenerateDrawCall(Render::DrawCallQueue& queue);
		static RenderScene* GetDefaultScene(); // TODO TEST
		static void Clear() { if (s_Default) s_Default.Reset(); }
	private:
		static TUniquePtr<RenderScene> s_Default;
		TVector<RenderObject*> m_RenderObjects; // all render objects
		TUniquePtr<DirectionalLight> m_DirectionalLight;
		TUniquePtr<Camera> m_Camera;
		RHIBufferPtr m_UniformBuffer;
		void UpdateUniform();
		void CreateResources();
	};
}
