#pragma once
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "Core/Public/Defines.h"
#include "Core/Public/Log.h"

#define VK_ASSERT(x, s)\
	ASSERT(VK_SUCCESS == (x), s)

#define VK_CHECK(x) (VK_SUCCESS == (x)

const uint32 VK_API_VER = VK_API_VERSION_1_2;

constexpr uint32 VK_INVALID_IDX = UINT32_MAX;

typedef void* WindowHandle;

// List of all Vulkan entry points
#define ENUM_VULKAN_LOADER_FUNCTIONS(macroName) \
	macroName(PFN_vkGetInstanceProcAddr                      , vkGetInstanceProcAddr)\
	macroName(PFN_vkCreateInstance                           , vkCreateInstance)\
	macroName(PFN_vkEnumerateInstanceExtensionProperties     , vkEnumerateInstanceExtensionProperties)\
	macroName(PFN_vkEnumerateInstanceLayerProperties         , vkEnumerateInstanceLayerProperties)\
	macroName(PFN_vkEnumerateInstanceVersion                 , vkEnumerateInstanceVersion)\

#define ENUM_VULKAN_INSTANCE_FUNCTIONS(macroName) \
	macroName(PFN_vkCreateDevice                             , vkCreateDevice)\
	macroName(PFN_vkDestroyDebugUtilsMessengerEXT            , vkDestroyDebugUtilsMessengerEXT)\
	macroName(PFN_vkCreateDebugUtilsMessengerEXT             , vkCreateDebugUtilsMessengerEXT)\
	macroName(PFN_vkDestroyInstance                          , vkDestroyInstance)\
	macroName(PFN_vkEnumerateDeviceExtensionProperties       , vkEnumerateDeviceExtensionProperties)\
	macroName(PFN_vkEnumerateDeviceLayerProperties           , vkEnumerateDeviceLayerProperties)\
	macroName(PFN_vkEnumeratePhysicalDevices                 , vkEnumeratePhysicalDevices)\
	macroName(PFN_vkGetDeviceQueue                           , vkGetDeviceQueue)\
	macroName(PFN_vkGetPhysicalDeviceProperties              , vkGetPhysicalDeviceProperties)\
	macroName(PFN_vkGetPhysicalDeviceProperties2             , vkGetPhysicalDeviceProperties2)\
	macroName(PFN_vkGetPhysicalDeviceQueueFamilyProperties   , vkGetPhysicalDeviceQueueFamilyProperties)\
	macroName(PFN_vkGetPhysicalDeviceFormatProperties        , vkGetPhysicalDeviceFormatProperties)\
	macroName(PFN_vkGetPhysicalDeviceMemoryProperties        , vkGetPhysicalDeviceMemoryProperties)\
	macroName(PFN_vkGetDeviceProcAddr                        , vkGetDeviceProcAddr)\
	macroName(PFN_vkGetPhysicalDeviceSurfaceFormatsKHR       , vkGetPhysicalDeviceSurfaceFormatsKHR)\
	macroName(PFN_vkGetPhysicalDeviceSurfacePresentModesKHR  , vkGetPhysicalDeviceSurfacePresentModesKHR)\
	macroName(PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR  , vkGetPhysicalDeviceSurfaceCapabilitiesKHR)\
	macroName(PFN_vkGetPhysicalDeviceFeatures                , vkGetPhysicalDeviceFeatures)\
	macroName(PFN_vkGetPhysicalDeviceFeatures2               , vkGetPhysicalDeviceFeatures2)\
	macroName(PFN_vkGetPhysicalDeviceSurfaceSupportKHR       , vkGetPhysicalDeviceSurfaceSupportKHR)\
	macroName(PFN_vkCreateCommandPool                        , vkCreateCommandPool)\

#define ENUM_VULKAN_DEVICE_FUNCTIONS(macroName) \
	macroName(PFN_vkCreateFence                              , vkCreateFence)\
	macroName(PFN_vkDestroyFence                             , vkDestroyFence)\
	macroName(PFN_vkWaitForFences                            , vkWaitForFences)\
	macroName(PFN_vkResetFences                              , vkResetFences)\
	macroName(PFN_vkResetCommandPool                         , vkResetCommandPool)\
	macroName(PFN_vkDestroyCommandPool                       , vkDestroyCommandPool)\
	macroName(PFN_vkBeginCommandBuffer                       , vkBeginCommandBuffer)\
	macroName(PFN_vkEndCommandBuffer                         , vkEndCommandBuffer)\
	macroName(PFN_vkAllocateCommandBuffers                   , vkAllocateCommandBuffers)\
	macroName(PFN_vkFreeCommandBuffers                       , vkFreeCommandBuffers)\
	macroName(PFN_vkCmdBeginRenderPass                       , vkCmdBeginRenderPass)\
	macroName(PFN_vkCmdCopyBufferToImage                     , vkCmdCopyBufferToImage)\
	macroName(PFN_vkCmdBlitImage                             , vkCmdBlitImage)\
	macroName(PFN_vkCmdCopyImage                             , vkCmdCopyImage)\
	macroName(PFN_vkCmdNextSubpass                           , vkCmdNextSubpass)\
	macroName(PFN_vkCmdEndRenderPass                         , vkCmdEndRenderPass)\
	macroName(PFN_vkCmdBeginRendering                        , vkCmdBeginRendering)\
	macroName(PFN_vkCmdEndRendering                          , vkCmdEndRendering)\
	macroName(PFN_vkCmdBindPipeline                          , vkCmdBindPipeline)\
	macroName(PFN_vkCmdSetViewport                           , vkCmdSetViewport)\
	macroName(PFN_vkCmdSetScissor                            , vkCmdSetScissor)\
	macroName(PFN_vkCmdBindVertexBuffers                     , vkCmdBindVertexBuffers)\
	macroName(PFN_vkCmdBindIndexBuffer                       , vkCmdBindIndexBuffer)\
	macroName(PFN_vkCmdBindDescriptorSets                    , vkCmdBindDescriptorSets)\
	macroName(PFN_vkCmdDraw                                  , vkCmdDraw)\
	macroName(PFN_vkCmdDrawIndexed                           , vkCmdDrawIndexed)\
	macroName(PFN_vkCmdDrawIndirect						     , vkCmdDrawIndirect)\
	macroName(PFN_vkCmdDrawIndexedIndirect				     , vkCmdDrawIndexedIndirect)\
	macroName(PFN_vkCmdClearAttachments                      , vkCmdClearAttachments)\
	macroName(PFN_vkCmdDispatch                              , vkCmdDispatch)\
	macroName(PFN_vkCmdCopyBuffer                            , vkCmdCopyBuffer)\
	macroName(PFN_vkCmdBeginDebugUtilsLabelEXT               , vkCmdBeginDebugUtilsLabelEXT)\
	macroName(PFN_vkCmdEndDebugUtilsLabelEXT                 , vkCmdEndDebugUtilsLabelEXT)\
	macroName(PFN_vkQueueSubmit                              , vkQueueSubmit)\
	macroName(PFN_vkQueuePresentKHR                          , vkQueuePresentKHR)\
	macroName(PFN_vkQueueWaitIdle                            , vkQueueWaitIdle)\
	macroName(PFN_vkCreateRenderPass                         , vkCreateRenderPass)\
	macroName(PFN_vkDestroyRenderPass                        , vkDestroyRenderPass)\
	macroName(PFN_vkAllocateMemory                           , vkAllocateMemory)\
	macroName(PFN_vkFreeMemory                               , vkFreeMemory)\
	macroName(PFN_vkCreateBuffer                             , vkCreateBuffer)\
	macroName(PFN_vkDestroyBuffer                            , vkDestroyBuffer)\
	macroName(PFN_vkCreateSampler                            , vkCreateSampler)\
	macroName(PFN_vkDestroySampler                           , vkDestroySampler)\
	macroName(PFN_vkCreateImage                              , vkCreateImage)\
	macroName(PFN_vkDestroyImage                             , vkDestroyImage)\
	macroName(PFN_vkCreateImageView                          , vkCreateImageView)\
	macroName(PFN_vkDestroyImageView                         , vkDestroyImageView)\
	macroName(PFN_vkCreateFramebuffer                        , vkCreateFramebuffer)\
	macroName(PFN_vkDestroyFramebuffer                       , vkDestroyFramebuffer)\
	macroName(PFN_vkCmdPipelineBarrier                       , vkCmdPipelineBarrier)\
	macroName(PFN_vkCreateDescriptorPool                     , vkCreateDescriptorPool)\
	macroName(PFN_vkDestroyDescriptorPool                    , vkDestroyDescriptorPool)\
	macroName(PFN_vkAllocateDescriptorSets                   , vkAllocateDescriptorSets)\
	macroName(PFN_vkUpdateDescriptorSets                     , vkUpdateDescriptorSets)\
	macroName(PFN_vkFreeDescriptorSets                       , vkFreeDescriptorSets)\
	macroName(PFN_vkCreateDescriptorSetLayout                , vkCreateDescriptorSetLayout)\
	macroName(PFN_vkDestroyDescriptorSetLayout               , vkDestroyDescriptorSetLayout)\
	macroName(PFN_vkCreateSemaphore                          , vkCreateSemaphore)\
	macroName(PFN_vkDestroySemaphore                         , vkDestroySemaphore)\
	macroName(PFN_vkCreateSwapchainKHR                       , vkCreateSwapchainKHR)\
	macroName(PFN_vkGetSwapchainImagesKHR                    , vkGetSwapchainImagesKHR)\
	macroName(PFN_vkDestroySwapchainKHR                      , vkDestroySwapchainKHR)\
	macroName(PFN_vkDestroyDevice                            , vkDestroyDevice)\
	macroName(PFN_vkAcquireNextImageKHR                      , vkAcquireNextImageKHR)\
	macroName(PFN_vkCreateComputePipelines                   , vkCreateComputePipelines)\
	macroName(PFN_vkCreateGraphicsPipelines                  , vkCreateGraphicsPipelines)\
	macroName(PFN_vkCreatePipelineLayout                     , vkCreatePipelineLayout)\
	macroName(PFN_vkDestroyPipelineLayout                    , vkDestroyPipelineLayout)\
	macroName(PFN_vkDestroyPipeline                          , vkDestroyPipeline)\
	macroName(PFN_vkCreateShaderModule                       , vkCreateShaderModule)\
	macroName(PFN_vkDestroyShaderModule                      , vkDestroyShaderModule)\
	macroName(PFN_vkSetDebugUtilsObjectNameEXT               , vkSetDebugUtilsObjectNameEXT)\

#define DECLARE_VULKAN_FUNCTION(type, name) extern type name;
ENUM_VULKAN_LOADER_FUNCTIONS(DECLARE_VULKAN_FUNCTION)\
ENUM_VULKAN_INSTANCE_FUNCTIONS(DECLARE_VULKAN_FUNCTION)\
ENUM_VULKAN_DEVICE_FUNCTIONS(DECLARE_VULKAN_FUNCTION)\

bool InitializeInstanceProcAddr();
void LoadInstanceFunctions(VkInstance instance);
void LoadDeviceFunctions(VkDevice device);

const char** GetDeviceExtensions(uint32* count);
