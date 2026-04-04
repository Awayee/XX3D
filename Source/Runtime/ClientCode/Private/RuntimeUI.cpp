#include "ClientCode/Public/RuntimeUI.h"
#include "Math/Public/Math.h"
#include "System/Public/Timer.h"
#include "RHI/Public/RHIImGui.h"
#include "RHI/Public/RHI.h"
#include "System/Public/ConfigManager.h"
#include "Window/Public/EngineWindow.h"


namespace Runtime {
	void RuntimeUIMgr::InitializeImGuiConfig() {
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigWindowsMoveFromTitleBarOnly = true;
		io.IniFilename = nullptr; // Do not save settings
		// load font
		const FSize2D scale = Engine::EngineWindow::Instance()->GetWindowContentScale();
		const float contentScale = Math::Max(1.0f, Math::Max(scale.w, scale.h));
		const XString fontFile = Engine::EngineConfig_Old::Instance().GetEngineAssetDir().append(Engine::EngineConfig_Old::Instance().GetDefaultFont()).string();
		io.Fonts->AddFontFromFileTTF(fontFile.c_str(), contentScale * 16, nullptr, nullptr);
		ASSERT(io.Fonts->Build(), "Failed to build fonts");
		io.IniFilename = nullptr; // Do not save settings
	}

	void RuntimeUIMgr::Tick() {
		RHIImGui::Instance()->FrameBegin();
		UpdateUI();
		RHIImGui::Instance()->FrameEnd();
	}

	bool RuntimeUIMgr::NeedDraw() {
		return m_NeedDraw;
	}

	RuntimeUIMgr::RuntimeUIMgr() : m_NeedDraw(false) {
		Engine::EngineWindow::Instance()->RegisterOnKeyFunc([this](Engine::EKey key, Engine::EInput input) {
			if(Engine::EInput::Press == input && Engine::EKey::Escape == key) {
				m_NeedDraw = !m_NeedDraw;
			}
		});
	}


	void RuntimeUIMgr::UpdateUI() {
		if (m_NeedDraw) {
			ImGui::Begin("Debug");
			// display fps
			uint32 fps = (uint32)Engine::Timer::GetFPS();
			ImGui::Text("FPS=%u", fps);
			// capture frame
			if (ImGui::Button("CaptureFrame")) {
				RHI::Instance()->CaptureFrame();
			}
			ImGui::End();
		}
	}
}
