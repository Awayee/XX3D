#include "RHI/Public/ImGuiRHI.h"

TUniquePtr<ImGuiRHI> ImGuiRHI::s_Instance;

ImGuiRHI* ImGuiRHI::Instance() {
	return s_Instance.Get();
}

void ImGuiRHI::Initialize() {
}

void ImGuiRHI::Release() {
}
