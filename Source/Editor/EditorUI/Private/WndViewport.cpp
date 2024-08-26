#include "WndViewport.h"
#include "EditorUI/Public/EditorUIMgr.h"
#include "Functions/Public/EditorLevelMgr.h"
#include "Objects/Public/RenderScene.h"
#include "Objects/Public/Camera.h"
#include "Objects/Public/DirectionalLight.h"
#include "Render/Public/DefaultResource.h"
#include "System/Public/Timer.h"

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
				Math::FQuaternion::AngleAxis(z * 0.004f, forward) *
				Math::FQuaternion::AngleAxis(y * 0.004f, up) *
				Math::FQuaternion::AngleAxis(x * 0.004f, right);
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
		if(m_RenderTarget) {
			ImGuiRHI::Instance()->RemoveImGuiTexture(m_RenderTargetID);
		}
	}

	void WndViewport::CameraControl(Object::RenderCamera* camera) {
		if(!ImGui::IsWindowFocused()) {
			return;
		}
		ImGuiMouseButton btn = ImGuiMouseButton_Right;
		if (ImGui::IsMouseClicked(btn)) {
			auto pos = ImGui::GetMousePos();
			m_LastX = pos.x;
			m_LastY = pos.y;
			m_MouseDown = true;
		}
		if (ImGui::IsMouseReleased(btn)) {
			m_MouseDown = false;
			m_LastX = 0.0f;
			m_LastY = 0.0f;
		}
		
		//rotate
		if (m_MouseDown) {
			auto pos = ImGui::GetMousePos();
			float x = pos.x, y = pos.y;
			if (m_LastX == 0.0f && m_LastY == 0.0f) {
				m_LastX = x;
				m_LastY = y;
			}
			else {
				float dX = x - m_LastX;
				float dY = y - m_LastY;
				RotateCamera(camera, dY, dX, 0.0f, true);
				m_LastX = x;
				m_LastY = y;
			}
		}

		//move
		{
			int x = ImGui::IsKeyDown(ImGuiKey_D) - ImGui::IsKeyDown(ImGuiKey_A);
			int z = ImGui::IsKeyDown(ImGuiKey_W) - ImGui::IsKeyDown(ImGuiKey_S);
			int y = ImGui::IsKeyDown(ImGuiKey_E) - ImGui::IsKeyDown(ImGuiKey_Q);
			if (x || y || z) {
				MoveCamera(camera, (float)x * 0.004f, (float)y * 0.004f, (float)z * 0.004f, true);
			}
		}
	}

	void WndViewport::Update() {
		// if this window is hided, disable the main pass rendering
		if (!m_Enable && m_ViewportShow) {
			if(EditorLevel* level = Editor::EditorLevelMgr::Instance()->GetLevel()) {
				if(Object::RenderScene* scene = level->GetScene()) {
					scene->SetRenderTarget(nullptr, {});
				}
			}
			ReleaseRenderTarget();
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
		auto size = ImGui::GetWindowSize();

		if (!m_ViewportShow || !Math::FloatEqual(size.x, m_ViewportSize.w) || !Math::FloatEqual(size.y, m_ViewportSize.h)) {
			m_ViewportSize = { (uint32)size.x, (uint32)size.y };
			ReleaseRenderTarget();
			CreateRenderTarget();
			scene->SetViewportSize(m_ViewportSize);
			scene->SetRenderTarget(m_RenderTarget.Get(), {});
			m_ViewportShow = true;
		}
		if (m_RenderTarget) {
			ImGui::SetCursorPos(ImVec2(0, 0));
			ImGui::Image(m_RenderTargetID, size);
			// show fps
			ImGui::SetCursorPos(ImVec2(32, 32));
			ImGui::Text("FPS = %u", static_cast<uint32>(Engine::CTimer::Instance()->GetFPS()));
		}
	}

	void WndViewport::ReleaseRenderTarget() {
		if (m_RenderTarget) {
			Render::ViewportRenderer::Instance()->WaitAllFence();
			ImGuiRHI::Instance()->RemoveImGuiTexture(m_RenderTargetID);
			m_RenderTarget.Reset();
		}
	}

	void WndViewport::CreateRenderTarget() {
		if(m_ViewportSize.w > 0 && m_ViewportSize.h > 0) {
			RHITextureDesc desc = RHITextureDesc::Texture2D();
			desc.Format = ERHIFormat::B8G8R8A8_UNORM;
			desc.Flags = TEXTURE_FLAG_SRV | TEXTURE_FLAG_COLOR_TARGET;
			desc.Width = m_ViewportSize.w;
			desc.Height = m_ViewportSize.h;
			m_RenderTarget = RHI::Instance()->CreateTexture(desc);
			RHISampler* sampler = Render::DefaultResources::Instance()->GetDefaultSampler(ESamplerFilter::Bilinear, ESamplerAddressMode::Clamp);
			m_RenderTargetID = ImGuiRHI::Instance()->RegisterImGuiTexture(m_RenderTarget.Get(), sampler);
		}
	}
}
