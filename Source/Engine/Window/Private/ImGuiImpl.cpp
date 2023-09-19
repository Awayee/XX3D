#include "Core/Public/macro.h"
#include "Window/Public/ImGuiImpl.h"
#include "Resource/Public/Config.h"
#include "RHI/Public/RHI.h"
#include "RHI/Private/RHIVulkan/RHIVulkan.h"

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

namespace Engine {

#define BRANCH_VULKAN \
	if(RHI_VULKAN == GetConfig().RHIType) {

#define BRANCH_END return ;}

	// check ImGui::NewFrame() is called before Draw
	static bool s_FrameFlag{ false };
	static ImGuiContext* s_ImGuiContext{ nullptr };


	void ImGuiInitialize(Engine::RRenderPass* pass, uint32 subpass)
	{
		IMGUI_CHECKVERSION();
		s_ImGuiContext = ImGui::CreateContext();

		BRANCH_VULKAN
		Engine::RHIVulkan* vkRHI = reinterpret_cast<Engine::RHIVulkan*>(RHI::Instance());
		Engine::RSVkImGuiInitInfo initInfo = vkRHI->GetImGuiInitInfo();
		ImGui_ImplGlfw_InitForVulkan(reinterpret_cast<GLFWwindow*>(initInfo.windowHandle), true);
		ImGui_ImplVulkan_InitInfo imGuiInit = {};
		imGuiInit.Instance = initInfo.instance;
		imGuiInit.PhysicalDevice = initInfo.physicalDevice;
		imGuiInit.Device = initInfo.device;
		imGuiInit.QueueFamily = initInfo.queueIndex;
		imGuiInit.Queue = initInfo.queue;
		imGuiInit.DescriptorPool = initInfo.descriptorPool;
		imGuiInit.Subpass = subpass;
		// may be different from the real swapchain image count
		// see ImGui_ImplVulkanH_GetMinImageCountFromPresentMode
		imGuiInit.MinImageCount = 3;
		imGuiInit.ImageCount = 3;
		ASSERT(ImGui_ImplVulkan_Init(&imGuiInit, reinterpret_cast<Engine::RRenderPassVk*>(pass)->handle), "Failed to init imgui!");

		BRANCH_END
	}
	void ImGuiRelease()
	{
		if(!s_ImGuiContext) {
			return;
		}
		BRANCH_VULKAN
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		BRANCH_END
		ImGui::DestroyContext(s_ImGuiContext); // todo solve
	}
	void ImGuiRenderDrawData(Engine::RCommandBuffer* cmd) {
		if(!s_FrameFlag) {
			return;
		}
		s_FrameFlag = false;
		ImGui::Render();
		BRANCH_VULKAN ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), reinterpret_cast<Engine::RCommandBufferVk*>(cmd)->handle); BRANCH_END
	}
	void ImGuiCreateFontsTexture(Engine::RCommandBuffer* cmd)
	{
		BRANCH_VULKAN ASSERT(ImGui_ImplVulkan_CreateFontsTexture(reinterpret_cast<Engine::RCommandBufferVk*>(cmd)->handle), "Failed to upload imgui fonts!"); BRANCH_END
	}
	void ImGuiNewFrame() {
		BRANCH_VULKAN
		ImGui_ImplGlfw_NewFrame();
		ImGui_ImplVulkan_NewFrame();
		ImGui::NewFrame();
		s_FrameFlag = true;
		BRANCH_END
	}

	void ImGuiEndFrame() {
		ImGui::EndFrame();
	}

	void ImGuiDestroyFontUploadObjects()
	{
		BRANCH_VULKAN ImGui_ImplVulkan_DestroyFontUploadObjects(); BRANCH_END
	}
}
