#include "VulkanImGui.h"
#include "VulkanRHI.h"
#include "VulkanDescriptorSet.h"
#include "VulkanSwapchain.h"
#include "VulkanCommand.h"
#include "Window/Public/EngineWIndow.h"
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

VulkanImGui::VulkanImGui() {
	IMGUI_CHECKVERSION();
	m_Context = ImGui::CreateContext();

	VulkanRHI* vkRHI = reinterpret_cast<VulkanRHI*>(RHI::Instance());
	WindowHandle windowHandle = Engine::EngineWindow::Instance()->GetWindowHandle();
	const VulkanContext* context = vkRHI->GetContext();
	VulkanDevice* device = vkRHI->GetDevice();
	ImGui_ImplGlfw_InitForVulkan(reinterpret_cast<GLFWwindow*>(windowHandle), true);
	ImGui_ImplVulkan_InitInfo initInfo {};
	initInfo.Instance = context->GetInstance();
	initInfo.PhysicalDevice = device->GetPhysicalDevice();
	initInfo.Device = device->GetDevice();
	initInfo.QueueFamily = device->GetGraphicsQueue().FamilyIndex;
	initInfo.Queue = device->GetGraphicsQueue().Handle;
	initInfo.DescriptorPool = device->GetDescriptorMgr()->GetPool();
	initInfo.MinImageCount = vkRHI->GetSwapchain()->GetImageCount();
	initInfo.ImageCount = initInfo.MinImageCount;
	initInfo.UseDynamicRendering = true;
	//dynamic rendering parameters for imgui to use
	initInfo.PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	VkFormat format = vkRHI->GetSwapchain()->GetSwapchainFormat();
	initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
	ImGui_ImplVulkan_Init(&initInfo);
	m_IsInitialized = false;
}

VulkanImGui::~VulkanImGui() {
	if (m_Context) {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext(m_Context); // todo solve
		m_Context = nullptr;
	}
}
void VulkanImGui::FrameBegin() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();
	m_FrameBegin = true;
}

void VulkanImGui::FrameEnd() {
	ImGui::EndFrame();
}

void VulkanImGui::RenderDrawData(RHICommandBuffer* cmd) {
	if (!m_FrameBegin) {
		return;
	}
	m_FrameBegin = false;
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VulkanRHICommandBuffer*>(cmd)->GetHandle());
}

void VulkanImGui::ImGuiCreateFontsTexture(RHICommandBuffer* cmd) {
	//ImGui_ImplVulkan_CreateFontsTexture(static_cast<VulkanRHICommandBuffer*>(cmd)->GetHandle());
}

void VulkanImGui::ImGuiDestroyFontUploadObjects() {
	//ImGui_ImplVulkan_DestroyFontUploadObjects();
}
