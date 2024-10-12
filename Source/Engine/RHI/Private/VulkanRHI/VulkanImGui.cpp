#include "VulkanImGui.h"
#include "VulkanRHI.h"
#include "VulkanPipeline.h"
#include "VulkanViewport.h"
#include "VulkanCommand.h"
#include "VulkanConverter.h"
#include "Window/Public/EngineWindow.h"
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

PFN_vkVoidFunction GetVkDeviceFunction(const char* name, void* userData) {
	VulkanRHI* r = (VulkanRHI*)VulkanRHI::Instance();
	auto func = vkGetDeviceProcAddr(r->GetDevice()->GetDevice(), name);
	if(!func) {
		func = vkGetInstanceProcAddr(r->GetContext()->GetInstance(), name);
	}
	ASSERT(func, "");
	return func;
}

VulkanImGui::VulkanImGui(void(*configInitializer)()) {
	IMGUI_CHECKVERSION();
	m_Context = ImGui::CreateContext();
	ImGui_ImplVulkan_LoadFunctions(GetVkDeviceFunction);
	VulkanRHI* vkRHI = reinterpret_cast<VulkanRHI*>(RHI::Instance());
	WindowHandle windowHandle = Engine::EngineWindow::Instance()->GetWindowHandle();
	const VulkanContext* context = vkRHI->GetContext();
	VulkanDevice* device = vkRHI->GetDevice();
	ImGui_ImplGlfw_InitForVulkan(reinterpret_cast<GLFWwindow*>(windowHandle), true);
	ImGui_ImplVulkan_InitInfo initInfo {};
	initInfo.Instance = context->GetInstance();
	initInfo.PhysicalDevice = device->GetPhysicalDevice();
	initInfo.Device = device->GetDevice();
	const VulkanQueue& queue = device->GetQueue(EQueueType::Graphics);
	initInfo.QueueFamily =queue.FamilyIndex;
	initInfo.Queue = queue.Handle;
	initInfo.DescriptorPool = device->GetDescriptorMgr()->GetReservePool();
	VulkanViewport* vkViewport = (VulkanViewport*)(vkRHI->GetViewport());
	initInfo.MinImageCount = vkViewport->GetImageCount();
	initInfo.ImageCount = initInfo.MinImageCount;
	initInfo.UseDynamicRendering = true;
	//dynamic rendering parameters for imgui to use
	initInfo.PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	VkFormat format = ToVkFormat(vkViewport->GetBackBufferFormat());
	initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
	ImGui_ImplVulkan_Init(&initInfo);
	if(configInitializer) {
		configInitializer();
	}
	ImGui_ImplVulkan_CreateFontsTexture();
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
}

void VulkanImGui::FrameEnd() {
	ImGui::EndFrame();
}

void VulkanImGui::RenderDrawData(RHICommandBuffer* cmd) {
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VulkanCommandBuffer*>(cmd)->GetHandle());
}

ImTextureID VulkanImGui::RegisterImGuiTexture(RHITexture* texture, RHITextureSubRes subRes, RHISampler* sampler) {
	VulkanRHITexture* vkTexture = (VulkanRHITexture*)texture;
	VulkanRHISampler* vkSampler = (VulkanRHISampler*)sampler;
	return ImGui_ImplVulkan_AddTexture(vkSampler->GetSampler(), vkTexture->GetView(subRes), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanImGui::RemoveImGuiTexture(ImTextureID textureID) {
	ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)textureID);
}
