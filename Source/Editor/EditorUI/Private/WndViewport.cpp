#include "WndViewport.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/EditorLevelMgr.h"
#include "Functions/Public/EditorConfig.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Render/Public/DefaultResource.h"
#include "System/Public/Timer.h"
#include "Window/Public/EngineWindow.h"

namespace Editor {

	void MoveCamera(Object::RenderCamera* camera, float x, float y, float z, bool local) {
		Math::FVector3 eye = camera->GetView().Eye;
		Math::FVector3 at = camera->GetView().At;
		Math::FVector3 up = camera->GetView().Up;
		Math::FVector3 delta;
		if (local) {
			Math::FVector3 forward = at - eye;
			Math::FVector3 right = Math::FVector3::Cross(up, forward);
			delta = right * x + forward * z + up * y;
		}
		else {
			delta = { x, y, z };
		}
		eye += delta;
		at += delta;
		camera->SetView({eye, at, up});
	}

	void RotateCamera(Object::RenderCamera* camera, float x, float y, float z, bool localSpace) {
		Math::FVector3 eye = camera->GetView().Eye;
		Math::FVector3 at = camera->GetView().At;
		Math::FVector3 tempUp = camera->GetView().Up;
		if (localSpace) {
			Math::FVector3 forward = at - eye;
			forward.NormalizeSelf();
			Math::FVector3 right = Math::FVector3::Cross(tempUp, forward);
			right.NormalizeSelf();
			Math::FVector3 up = Math::FVector3::Cross(forward, right);
			Math::FQuaternion rotateQuat =
				Math::FQuaternion::AngleAxis(z, forward) *
				Math::FQuaternion::AngleAxis(y, up) *
				Math::FQuaternion::AngleAxis(x, right);
			forward = rotateQuat.RotateVector3(forward);
			at = eye + forward;
		}
		else {
		}
		camera->SetView({eye, at, tempUp});
	}

	WndViewport::WndViewport() : EditorWndBase("Viewport", ImGuiWindowFlags_NoBackground) {
		EditorUIMgr::Instance()->AddMenu("Window", m_Name, {}, &m_Enable);
		EditorUIMgr::Instance()->AddMenu("Render", "Capture", []() {RHI::Instance()->CaptureFrame(); }, nullptr);
		m_Flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	}

	WndViewport::~WndViewport() {
		if(m_RenderTargetID) {
			RHIImGui::Instance()->RemoveImGuiTexture(m_RenderTargetID);
		}
	}

	void WndViewport::CameraControl(Object::RenderCamera* camera) {
		if(!ImGui::IsWindowFocused()) {
			return;
		}

		Engine::EngineWindow* window = Engine::EngineWindow::Instance();
		
		//rotate
		if(window->IsMouseDown(Engine::EBtn::Right)) {
			// firstly pressed, record mouse position
			if(!m_MouseDown) {
				const FOffset2D pos = window->GetCursorPos();
				m_LastX = pos.x;
				m_LastY = pos.y;
				m_MouseDown = true;
			}
			// rotate camera
			auto pos = Engine::EngineWindow::Instance()->GetCursorPos();
			float x = pos.x, y = pos.y;
			if (m_LastX == 0.0f && m_LastY == 0.0f) {
				m_LastX = x;
				m_LastY = y;
			}
			else {
				float dX = x - m_LastX;
				float dY = y - m_LastY;
				if(!Math::IsNearlyZero(dX) || !Math::IsNearlyZero(dY)) {
					const float scale = EditorConfig::GetCameraRotateSpeed() * Math::Deg2Rad;
					RotateCamera(camera, dY * scale, dX * scale, 0.0f, true);
					m_LastX = x;
					m_LastY = y;
				}
			}
		}
		else if(m_MouseDown){
			// mouse released
			m_LastX = 0.0f;
			m_LastY = 0.0f;
			m_MouseDown = false;
		}

		//move
		{
			int x = (int)window->IsKeyDown(Engine::EKey::D) - (int)window->IsKeyDown(Engine::EKey::A);
			int z = (int)window->IsKeyDown(Engine::EKey::W) - (int)window->IsKeyDown(Engine::EKey::S);
			int y = (int)window->IsKeyDown(Engine::EKey::E) - (int)window->IsKeyDown(Engine::EKey::Q);
			if (x || y || z) {
				const float dt = Engine::CTimer::Instance()->GetDeltaTime();
				const float scale = EditorConfig::GetCameraMoveSpeed() * (dt / 41.67f);
				MoveCamera(camera, (float)x * scale, (float)y * scale, (float)z * scale, true);
			}
		}
	}

	void WndViewport::Update() {
		// if this window is hided, disable the main pass rendering
		if (!m_Enable && m_ViewportShow) {
			Editor::EditorUIMgr::Instance()->SetSceneTexture(nullptr);
			m_ViewportShow = false;
		}
	}

	void WndViewport::WndContent() {
		EditorLevel* level = Editor::EditorLevelMgr::Instance()->GetLevel();
		if(!level) {
			return;
		}
		Object::RenderScene* scene = level->GetScene();
		if(!scene) {
			return;
		}
		Object::RenderCamera* camera = scene->GetMainCamera();
		CameraControl(camera);

		// check render size;
		auto size = ImGui::GetWindowSize();
		USize2D windowSize = { (uint32)size.x, (uint32)size.y };
		if(m_ViewportSize != windowSize || !m_ViewportShow) {
			m_ViewportSize = windowSize;
			SetupRenderTarget();
			camera->SetAspect((float)m_ViewportSize.w / (float)m_ViewportSize.h);
			m_ViewportShow = true;
		}
		ImGui::SetCursorPos(ImVec2(0, 0));
		ImGui::Image(m_RenderTargetID, size);
		// show fps
		ImGui::SetCursorPos(ImVec2(32, 32));
		const uint32 fps = (uint32)Engine::CTimer::Instance()->GetFPS();
		const bool isFpsLow = fps < 60u;
		const ImVec4 color{isFpsLow?1.0f:0.0f, isFpsLow?0.0f:1.0f, 0.0f, 1.0f};
		ImGui::TextColored(color, "FPS=%u", fps);
	}

	void WndViewport::SetupRenderTarget() {
		// check need recreate render target
		if(!m_RenderTarget || m_RenderTarget->GetDesc().Width != m_ViewportSize.w || m_RenderTarget->GetDesc().Height != m_ViewportSize.h) {
			if (m_RenderTargetID) {
				RHIImGui::Instance()->RemoveImGuiTexture(m_RenderTargetID);
				m_RenderTargetID = nullptr;
			}
			RHITextureDesc desc = RHITextureDesc::Texture2D();
			desc.Width = m_ViewportSize.w;
			desc.Height = m_ViewportSize.h;
			desc.Format = RHI::Instance()->GetViewport()->GetBackBufferFormat();
			desc.Flags = ETextureFlags::SRV | ETextureFlags::ColorTarget;
			m_RenderTarget = RHI::Instance()->CreateTexture(desc);
			RHISampler* sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
			m_RenderTargetID = RHIImGui::Instance()->RegisterImGuiTexture(m_RenderTarget, sampler);
		}
		
		Editor::EditorUIMgr::Instance()->SetSceneTexture(m_RenderTarget);
	}
}
