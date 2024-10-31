#pragma once
#include "Math/Public/Transform.h"
#include "Asset/Public/MeshAsset.h"
#include "Core/Public/TUniquePtr.h"
#include "Objects/Public/Level.h"

namespace Object {
	// The level components registered by REGISTER_LEVEL_COMPONENT handles game and editor logic;
	// the ECS components registered by REGISTER_ECS_COMPONENT handles rendering.

	class TransformComponent: public LevelComponent {
	public:
		Math::FTransform Transform;
		void OnLoad(const Json::Value& val) override;
		void OnAdd() override;
		void OnRemove() override;
		void TransformUpdated();
	private:
		REGISTER_LEVEL_COMPONENT(TransformComponent);
	};

	class MeshComponent: public LevelComponent {
	public:
		void OnLoad(const Json::Value& val) override;
		void OnAdd() override;
		void OnRemove() override;
		void SetMeshFile(const XString& meshFile);
		const XString& GetMeshFile() const { return m_MeshFile; }
		void SetCastShadow(bool bCastShadow);
		bool GetCastShadow() const { return m_CastShadow; }
	private:
		XString m_MeshFile;
		bool m_CastShadow;
		REGISTER_LEVEL_COMPONENT(MeshComponent);
	};

	class InstanceDataComponent: public LevelComponent {
	public:
		void OnLoad(const Json::Value& val) override;
		void OnAdd() override;
		void OnRemove() override;
		void SetInstanceFile(const XString& file);
		const XString& GetInstanceFile() const { return m_InstanceFile; }
	private:
		XString m_InstanceFile;
		REGISTER_LEVEL_COMPONENT(InstanceDataComponent);
	};
}
