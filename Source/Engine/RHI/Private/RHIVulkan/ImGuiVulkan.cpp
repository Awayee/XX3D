#include "ImGuiVulkan.h"
#include "RHIVulkan.h"
#include "VulkanDescriptorSet.h"
#include "VulkanSwapchain.h"
#include "VulkanCommand.h"
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

ImGuiVulkan::ImGuiVulkan(RHIRenderPass* pass) {
	IMGUI_CHECKVERSION();
	m_Context = ImGui::CreateContext();

	RHIVulkan* vkRHI = reinterpret_cast<RHIVulkan*>(RHI::Instance());
	const VulkanContext& context = vkRHI->GetContext();
	ImGui_ImplGlfw_InitForVulkan(reinterpret_cast<GLFWwindow*>(context.WindowHandle), true);
	ImGui_ImplVulkan_InitInfo imGuiInit = {};
	imGuiInit.Instance = context.Instance;
	imGuiInit.PhysicalDevice = context.PhysicalDevice;
	imGuiInit.Device = context.Device;
	imGuiInit.QueueFamily = context.GraphicsQueue.FamilyIndex;
	imGuiInit.Queue = context.GraphicsQueue.Handle;
	imGuiInit.DescriptorPool = VulkanDSMgr::Instance()->GetPool();
	imGuiInit.Subpass = static_cast<RHIVulkanRenderPass*>(pass)->GetSubPass();
	// may be different from the real swapchain image count
	// see ImGui_ImplVulkanH_GetMinImageCountFromPresentMode
	imGuiInit.MinImageCount = VulkanSwapchain::Instance()->GetImageCount();
	imGuiInit.ImageCount = imGuiInit.MinImageCount;
	ASSERT(ImGui_ImplVulkan_Init(&imGuiInit, static_cast<RHIVulkanRenderPass*>(pass)->GetRenderPass()), "Failed to init imgui!");
}

ImGuiVulkan::~ImGuiVulkan() {
	if (m_Context) {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext(m_Context); // todo solve
		m_Context = nullptr;
	}
}

void ImGuiVulkan::FrameBegin() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();
	m_FrameBegin = true;
}

void ImGuiVulkan::FrameEnd() {
	ImGui::EndFrame();
}

void ImGuiVulkan::RenderDrawData(RHICommandBuffer* cmd) {
	if (!m_FrameBegin) {
		return;
	}
	m_FrameBegin = false;
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<RHIVulkanCommandBuffer*>(cmd)->GetHandle());
}

void ImGuiVulkan::ImGuiCreateFontsTexture(RHICommandBuffer* cmd) {
	ImGui_ImplVulkan_CreateFontsTexture(static_cast<RHIVulkanCommandBuffer*>(cmd)->GetHandle());
}

void ImGuiVulkan::ImGuiDestroyFontUploadObjects() {
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}
