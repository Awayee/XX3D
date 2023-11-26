#include "VulkanCommon.h"


PFN_vkGetInstanceProcAddr                      vkGetInstanceProcAddr{ nullptr };// loader functions
PFN_vkCreateInstance                           vkCreateInstance{ nullptr };
PFN_vkEnumerateInstanceExtensionProperties     vkEnumerateInstanceExtensionProperties{ nullptr };
PFN_vkEnumerateInstanceLayerProperties         vkEnumerateInstanceLayerProperties{ nullptr };
PFN_vkEnumerateInstanceVersion                 vkEnumerateInstanceVersion{ nullptr };
PFN_vkCreateDevice                             vkCreateDevice{ nullptr };// instance functions	
PFN_vkDestroyInstance                          vkDestroyInstance{ nullptr };
PFN_vkEnumerateDeviceExtensionProperties       vkEnumerateDeviceExtensionProperties{ nullptr };
PFN_vkEnumerateDeviceLayerProperties           vkEnumerateDeviceLayerProperties{ nullptr };
PFN_vkEnumeratePhysicalDevices                 vkEnumeratePhysicalDevices{ nullptr };
PFN_vkGetDeviceQueue                           vkGetDeviceQueue{ nullptr };
PFN_vkGetPhysicalDeviceProperties              vkGetPhysicalDeviceProperties{ nullptr };
PFN_vkGetPhysicalDeviceQueueFamilyProperties   vkGetPhysicalDeviceQueueFamilyProperties{ nullptr };
PFN_vkGetPhysicalDeviceFormatProperties        vkGetPhysicalDeviceFormatProperties{ nullptr };
PFN_vkGetPhysicalDeviceMemoryProperties        vkGetPhysicalDeviceMemoryProperties{ nullptr };
PFN_vkGetDeviceProcAddr                        vkGetDeviceProcAddr{ nullptr };
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR       vkGetPhysicalDeviceSurfaceFormatsKHR{ nullptr };
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR  vkGetPhysicalDeviceSurfacePresentModesKHR{ nullptr };
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR  vkGetPhysicalDeviceSurfaceCapabilitiesKHR{ nullptr };
PFN_vkGetPhysicalDeviceFeatures                vkGetPhysicalDeviceFeatures{ nullptr };
PFN_vkGetPhysicalDeviceSurfaceSupportKHR       vkGetPhysicalDeviceSurfaceSupportKHR{ nullptr };
PFN_vkCreateDebugUtilsMessengerEXT             vkCreateDebugUtilsMessengerEXT{ nullptr };
PFN_vkDestroyDebugUtilsMessengerEXT            vkDestroyDebugUtilsMessengerEXT{ nullptr };
PFN_vkCreateCommandPool                        vkCreateCommandPool{ nullptr };// device functions
PFN_vkCreateFence                              vkCreateFence{ nullptr };
PFN_vkDestroyFence                             vkDestroyFence{ nullptr };
PFN_vkWaitForFences                            vkWaitForFences{ nullptr };
PFN_vkResetFences                              vkResetFences{ nullptr };
PFN_vkResetCommandPool                         vkResetCommandPool{ nullptr };
PFN_vkDestroyCommandPool                       vkDestroyCommandPool{ nullptr };
PFN_vkBeginCommandBuffer                       vkBeginCommandBuffer{ nullptr };
PFN_vkEndCommandBuffer                         vkEndCommandBuffer{ nullptr };
PFN_vkAllocateCommandBuffers                   vkAllocateCommandBuffers{ nullptr };
PFN_vkFreeCommandBuffers                       vkFreeCommandBuffers{ nullptr };
PFN_vkCmdBeginRenderPass                       vkCmdBeginRenderPass{ nullptr };
PFN_vkCmdCopyBufferToImage                     vkCmdCopyBufferToImage{ nullptr };
PFN_vkCmdBlitImage                             vkCmdBlitImage{ nullptr };
PFN_vkCmdCopyImage							   vkCmdCopyImage{ nullptr };
PFN_vkCmdNextSubpass                           vkCmdNextSubpass{ nullptr };
PFN_vkCmdEndRenderPass                         vkCmdEndRenderPass{ nullptr };
PFN_vkCmdBindPipeline                          vkCmdBindPipeline{ nullptr };
PFN_vkCmdSetViewport                           vkCmdSetViewport{ nullptr };
PFN_vkCmdSetScissor                            vkCmdSetScissor{ nullptr };
PFN_vkCmdBindVertexBuffers                     vkCmdBindVertexBuffers{ nullptr };
PFN_vkCmdBindIndexBuffer                       vkCmdBindIndexBuffer{ nullptr };
PFN_vkCmdBindDescriptorSets                    vkCmdBindDescriptorSets{ nullptr };
PFN_vkCmdDraw                                  vkCmdDraw{ nullptr };
PFN_vkCmdDrawIndexed                           vkCmdDrawIndexed{ nullptr };
PFN_vkCmdDrawIndirect						   vkCmdDrawIndirect{ nullptr };
PFN_vkCmdDrawIndexedIndirect				   vkCmdDrawIndexedIndirect{ nullptr };
PFN_vkCmdClearAttachments                      vkCmdClearAttachments{ nullptr };
PFN_vkCmdDispatch                              vkCmdDispatch{ nullptr };
PFN_vkCmdCopyBuffer                            vkCmdCopyBuffer{ nullptr };
PFN_vkCmdBeginDebugUtilsLabelEXT               vkCmdBeginDebugUtilsLabelEXT{ nullptr };
PFN_vkCmdEndDebugUtilsLabelEXT                 vkCmdEndDebugUtilsLabelEXT{ nullptr };
PFN_vkQueueSubmit                              vkQueueSubmit{ nullptr };
PFN_vkQueuePresentKHR                          vkQueuePresentKHR{ nullptr };
PFN_vkQueueWaitIdle                            vkQueueWaitIdle{ nullptr };
PFN_vkCreateRenderPass                         vkCreateRenderPass{ nullptr };
PFN_vkDestroyRenderPass                        vkDestroyRenderPass{ nullptr };
PFN_vkAllocateMemory                           vkAllocateMemory{ nullptr };
PFN_vkFreeMemory                               vkFreeMemory{ nullptr };
PFN_vkCreateBuffer                             vkCreateBuffer{ nullptr };
PFN_vkDestroyBuffer                            vkDestroyBuffer{ nullptr };
PFN_vkCreateSampler                            vkCreateSampler{ nullptr };
PFN_vkDestroySampler                           vkDestroySampler{ nullptr };
PFN_vkCreateImage                              vkCreateImage{ nullptr };
PFN_vkDestroyImage                             vkDestroyImage{ nullptr };
PFN_vkCreateImageView                          vkCreateImageView{ nullptr };
PFN_vkDestroyImageView                         vkDestroyImageView{ nullptr };
PFN_vkCreateFramebuffer                        vkCreateFramebuffer{ nullptr };
PFN_vkDestroyFramebuffer                       vkDestroyFramebuffer{ nullptr };
PFN_vkCmdPipelineBarrier                       vkCmdPipelineBarrier{ nullptr };
PFN_vkCreateDescriptorPool                     vkCreateDescriptorPool{ nullptr };
PFN_vkDestroyDescriptorPool                    vkDestroyDescriptorPool{ nullptr };
PFN_vkAllocateDescriptorSets                   vkAllocateDescriptorSets{ nullptr };
PFN_vkUpdateDescriptorSets                     vkUpdateDescriptorSets{ nullptr };
PFN_vkFreeDescriptorSets                       vkFreeDescriptorSets{ nullptr };
PFN_vkCreateDescriptorSetLayout                vkCreateDescriptorSetLayout{ nullptr };
PFN_vkDestroyDescriptorSetLayout               vkDestroyDescriptorSetLayout{ nullptr };
PFN_vkCreateSemaphore                          vkCreateSemaphore{ nullptr };
PFN_vkDestroySemaphore                         vkDestroySemaphore{ nullptr };
PFN_vkCreateSwapchainKHR                       vkCreateSwapchainKHR{ nullptr };
PFN_vkGetSwapchainImagesKHR                    vkGetSwapchainImagesKHR{ nullptr };
PFN_vkDestroySwapchainKHR                      vkDestroySwapchainKHR{ nullptr };
PFN_vkDestroyDevice                            vkDestroyDevice{ nullptr };
PFN_vkAcquireNextImageKHR                      vkAcquireNextImageKHR{ nullptr };
PFN_vkCreateComputePipelines                   vkCreateComputePipelines{ nullptr };
PFN_vkCreateGraphicsPipelines                  vkCreateGraphicsPipelines{ nullptr };
PFN_vkCreatePipelineLayout                     vkCreatePipelineLayout{ nullptr };
PFN_vkDestroyPipelineLayout                    vkDestroyPipelineLayout{ nullptr };
PFN_vkDestroyPipeline                          vkDestroyPipeline{ nullptr };
PFN_vkCreateShaderModule                       vkCreateShaderModule{ nullptr };
PFN_vkDestroyShaderModule                      vkDestroyShaderModule{ nullptr };
PFN_vkSetDebugUtilsObjectNameEXT               vkSetDebugUtilsObjectNameEXT{ nullptr };

#include <Windows.h>
static const char* VK_LIBRARY = "vulkan-1.dll";

#define LOAD_GLOBAL_PROC_ADDR(x) x = reinterpret_cast<PFN_##x>(vkGetInstanceProcAddr(nullptr, #x))
#define LOAD_INSTANCE_PROC_ADDR(x) x = reinterpret_cast<PFN_##x>(vkGetInstanceProcAddr(instance, #x))
#define LOAD_DEVICE_PROC_ADDR(x) x = reinterpret_cast<PFN_##x>(vkGetDeviceProcAddr(device, #x))

bool InitializeInstanceProcAddr() {
	static HMODULE s_Lib = LoadLibraryA(VK_LIBRARY);
	if (s_Lib == nullptr) {
		return false;
	}
	vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(s_Lib, "vkGetInstanceProcAddr"));
#if defined(VK_VERSION_1_0)
	LOAD_GLOBAL_PROC_ADDR(vkCreateInstance);
	LOAD_GLOBAL_PROC_ADDR(vkEnumerateInstanceExtensionProperties);
	LOAD_GLOBAL_PROC_ADDR(vkEnumerateInstanceLayerProperties);

#endif // defined(VK_VERSION_1_0)
#if defined(VK_VERSION_1_1)
	LOAD_GLOBAL_PROC_ADDR(vkEnumerateInstanceVersion);
#endif // defined(VK_VERSION_1_1)
	return true;
}

void LoadInstanceFunctions(VkInstance instance) {
	LOAD_INSTANCE_PROC_ADDR(vkCreateDevice);
	LOAD_INSTANCE_PROC_ADDR(vkDestroyInstance);
	LOAD_INSTANCE_PROC_ADDR(vkEnumerateDeviceExtensionProperties);
	LOAD_INSTANCE_PROC_ADDR(vkEnumerateDeviceLayerProperties);
	LOAD_INSTANCE_PROC_ADDR(vkEnumeratePhysicalDevices);
	LOAD_INSTANCE_PROC_ADDR(vkGetDeviceQueue);
	LOAD_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceProperties);
	LOAD_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceQueueFamilyProperties);
	LOAD_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceFormatProperties);
	LOAD_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceMemoryProperties);
	LOAD_INSTANCE_PROC_ADDR(vkGetDeviceProcAddr);
	LOAD_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceSurfaceFormatsKHR);
	LOAD_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceSurfacePresentModesKHR);
	LOAD_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
	LOAD_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceFeatures);
	LOAD_INSTANCE_PROC_ADDR(vkGetPhysicalDeviceSurfaceSupportKHR);
	LOAD_INSTANCE_PROC_ADDR(vkCreateDebugUtilsMessengerEXT);
	LOAD_INSTANCE_PROC_ADDR(vkDestroyDebugUtilsMessengerEXT);
}

void LoadDeviceFunctions(VkDevice device) {
	LOAD_DEVICE_PROC_ADDR(vkCreateCommandPool);
	LOAD_DEVICE_PROC_ADDR(vkCreateFence);
	LOAD_DEVICE_PROC_ADDR(vkDestroyFence);
	LOAD_DEVICE_PROC_ADDR(vkWaitForFences);
	LOAD_DEVICE_PROC_ADDR(vkResetFences);
	LOAD_DEVICE_PROC_ADDR(vkResetCommandPool);
	LOAD_DEVICE_PROC_ADDR(vkDestroyCommandPool);
	LOAD_DEVICE_PROC_ADDR(vkBeginCommandBuffer);
	LOAD_DEVICE_PROC_ADDR(vkEndCommandBuffer);
	LOAD_DEVICE_PROC_ADDR(vkAllocateCommandBuffers);
	LOAD_DEVICE_PROC_ADDR(vkFreeCommandBuffers);
	LOAD_DEVICE_PROC_ADDR(vkCmdBeginRenderPass);
	LOAD_DEVICE_PROC_ADDR(vkCmdCopyBufferToImage);
	LOAD_DEVICE_PROC_ADDR(vkCmdBlitImage);
	LOAD_DEVICE_PROC_ADDR(vkCmdCopyImage);
	LOAD_DEVICE_PROC_ADDR(vkCmdNextSubpass);
	LOAD_DEVICE_PROC_ADDR(vkCmdEndRenderPass);
	LOAD_DEVICE_PROC_ADDR(vkCmdBindPipeline);
	LOAD_DEVICE_PROC_ADDR(vkCmdSetViewport);
	LOAD_DEVICE_PROC_ADDR(vkCmdSetScissor);
	LOAD_DEVICE_PROC_ADDR(vkCmdBindVertexBuffers);
	LOAD_DEVICE_PROC_ADDR(vkCmdBindIndexBuffer);
	LOAD_DEVICE_PROC_ADDR(vkCmdBindDescriptorSets);
	LOAD_DEVICE_PROC_ADDR(vkCmdDrawIndexed);
	LOAD_DEVICE_PROC_ADDR(vkCmdDrawIndirect);
	LOAD_DEVICE_PROC_ADDR(vkCmdDrawIndexedIndirect);
	LOAD_DEVICE_PROC_ADDR(vkCmdClearAttachments);
	LOAD_DEVICE_PROC_ADDR(vkCmdDraw);
	LOAD_DEVICE_PROC_ADDR(vkCmdDispatch);
	LOAD_DEVICE_PROC_ADDR(vkCmdCopyBuffer);
	LOAD_DEVICE_PROC_ADDR(vkCmdBeginDebugUtilsLabelEXT);
	LOAD_DEVICE_PROC_ADDR(vkCmdEndDebugUtilsLabelEXT);
	LOAD_DEVICE_PROC_ADDR(vkQueueSubmit);
	LOAD_DEVICE_PROC_ADDR(vkQueuePresentKHR);
	LOAD_DEVICE_PROC_ADDR(vkQueueWaitIdle);
	LOAD_DEVICE_PROC_ADDR(vkCreateRenderPass);
	LOAD_DEVICE_PROC_ADDR(vkDestroyRenderPass);
	LOAD_DEVICE_PROC_ADDR(vkAllocateMemory);
	LOAD_DEVICE_PROC_ADDR(vkFreeMemory);
	LOAD_DEVICE_PROC_ADDR(vkCreateBuffer);
	LOAD_DEVICE_PROC_ADDR(vkDestroyBuffer);
	LOAD_DEVICE_PROC_ADDR(vkCreateSampler);
	LOAD_DEVICE_PROC_ADDR(vkDestroySampler);
	LOAD_DEVICE_PROC_ADDR(vkCreateImage);
	LOAD_DEVICE_PROC_ADDR(vkDestroyImage);
	LOAD_DEVICE_PROC_ADDR(vkCreateImageView);
	LOAD_DEVICE_PROC_ADDR(vkDestroyImageView);
	LOAD_DEVICE_PROC_ADDR(vkCreateFramebuffer);
	LOAD_DEVICE_PROC_ADDR(vkDestroyFramebuffer);
	LOAD_DEVICE_PROC_ADDR(vkCmdPipelineBarrier);
	LOAD_DEVICE_PROC_ADDR(vkCreateDescriptorPool);
	LOAD_DEVICE_PROC_ADDR(vkDestroyDescriptorPool);
	LOAD_DEVICE_PROC_ADDR(vkAllocateDescriptorSets);
	LOAD_DEVICE_PROC_ADDR(vkUpdateDescriptorSets);
	LOAD_DEVICE_PROC_ADDR(vkFreeDescriptorSets);
	LOAD_DEVICE_PROC_ADDR(vkCreateDescriptorSetLayout);
	LOAD_DEVICE_PROC_ADDR(vkDestroyDescriptorSetLayout);
	LOAD_DEVICE_PROC_ADDR(vkCreateSemaphore);
	LOAD_DEVICE_PROC_ADDR(vkDestroySemaphore);
	LOAD_DEVICE_PROC_ADDR(vkCreateSwapchainKHR);
	LOAD_DEVICE_PROC_ADDR(vkGetSwapchainImagesKHR);
	LOAD_DEVICE_PROC_ADDR(vkDestroySwapchainKHR);
	LOAD_DEVICE_PROC_ADDR(vkDestroyDevice);
	LOAD_DEVICE_PROC_ADDR(vkAcquireNextImageKHR);
	LOAD_DEVICE_PROC_ADDR(vkCreateComputePipelines);
	LOAD_DEVICE_PROC_ADDR(vkCreateGraphicsPipelines);
	LOAD_DEVICE_PROC_ADDR(vkCreatePipelineLayout);
	LOAD_DEVICE_PROC_ADDR(vkDestroyPipelineLayout);
	LOAD_DEVICE_PROC_ADDR(vkDestroyPipeline);
	LOAD_DEVICE_PROC_ADDR(vkCreateShaderModule);
	LOAD_DEVICE_PROC_ADDR(vkDestroyShaderModule);
	LOAD_DEVICE_PROC_ADDR(vkSetDebugUtilsObjectNameEXT);
}
