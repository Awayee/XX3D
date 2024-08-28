#include "VulkanImGui.h"
#include "VulkanRHI.h"
#include "VulkanPipeline.h"
#include "VulkanViewport.h"
#include "VulkanCommand.h"
#include "Window/Public/EngineWIndow.h"
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

VulkanImGui::VulkanImGui(void(*configInitializer)()) {
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
	initInfo.DescriptorPool = device->GetDescriptorMgr()->GetReservePool();
	VulkanViewport* vkViewport = (VulkanViewport*)(vkRHI->GetViewport());
	initInfo.MinImageCount = vkViewport->GetImageCount();
	initInfo.ImageCount = initInfo.MinImageCount;
	initInfo.UseDynamicRendering = true;
	//dynamic rendering parameters for imgui to use
	initInfo.PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	VkFormat format = vkViewport->GetSwapchainFormat();
	initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
	ImGui_ImplVulkan_Init(&initInfo);
	if(configInitializer) {
		configInitializer();
	}
	ImGui_ImplVulkan_CreateFontsTexture();
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
	ASSERT(m_FrameBegin, "");
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VulkanCommandBuffer*>(cmd)->GetHandle());
}

ImTextureID VulkanImGui::RegisterImGuiTexture(RHITexture* texture, RHISampler* sampler) {
	VulkanRHITexture* vkTexture = (VulkanRHITexture*)texture;
	VulkanRHISampler* vkSampler = (VulkanRHISampler*)sampler;
	return ImGui_ImplVulkan_AddTexture(vkSampler->GetSampler(), vkTexture->GetDefaultView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanImGui::RemoveImGuiTexture(ImTextureID textureID) {
	ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)textureID);
}

void VulkanImGui::ImGuiCreateFontsTexture(RHICommandBuffer* cmd) {
	//ImGui_ImplVulkan_CreateFontsTexture(static_cast<VulkanCommandBuffer*>(cmd)->GetHandle());
}

void VulkanImGui::ImGuiDestroyFontUploadObjects() {
	//ImGui_ImplVulkan_DestroyFontUploadObjects();
}
