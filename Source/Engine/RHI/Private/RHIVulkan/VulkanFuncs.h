#pragma once
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

// function pointers

extern PFN_vkGetInstanceProcAddr                      vkGetInstanceProcAddr                      ;// loader functions
extern PFN_vkCreateInstance                           vkCreateInstance                           ;
extern PFN_vkEnumerateInstanceExtensionProperties     vkEnumerateInstanceExtensionProperties     ;
extern PFN_vkEnumerateInstanceLayerProperties         vkEnumerateInstanceLayerProperties         ;
extern PFN_vkEnumerateInstanceVersion                 vkEnumerateInstanceVersion                 ;
extern PFN_vkCreateDevice                             vkCreateDevice                             ;// instance functions	
extern PFN_vkDestroyInstance                          vkDestroyInstance                          ;
extern PFN_vkEnumerateDeviceExtensionProperties       vkEnumerateDeviceExtensionProperties       ;
extern PFN_vkEnumerateDeviceLayerProperties           vkEnumerateDeviceLayerProperties           ;
extern PFN_vkEnumeratePhysicalDevices                 vkEnumeratePhysicalDevices                 ;
extern PFN_vkGetDeviceQueue                           vkGetDeviceQueue                           ;
extern PFN_vkGetPhysicalDeviceProperties              vkGetPhysicalDeviceProperties              ;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties   vkGetPhysicalDeviceQueueFamilyProperties   ;
extern PFN_vkGetPhysicalDeviceFormatProperties        vkGetPhysicalDeviceFormatProperties        ;
extern PFN_vkGetDeviceProcAddr                        vkGetDeviceProcAddr                        ;
extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR       vkGetPhysicalDeviceSurfaceFormatsKHR       ;
extern PFN_vkGetPhysicalDeviceSurfacePresentModesKHR  vkGetPhysicalDeviceSurfacePresentModesKHR  ;
extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR  vkGetPhysicalDeviceSurfaceCapabilitiesKHR  ;
extern PFN_vkGetPhysicalDeviceFeatures                vkGetPhysicalDeviceFeatures                ;
extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR       vkGetPhysicalDeviceSurfaceSupportKHR       ;
extern PFN_vkCreateCommandPool                        vkCreateCommandPool                        ;// device functions
extern PFN_vkCreateFence                              vkCreateFence                              ;
extern PFN_vkDestroyFence                             vkDestroyFence                             ;
extern PFN_vkWaitForFences                            vkWaitForFences                            ;
extern PFN_vkResetFences                              vkResetFences                              ;
extern PFN_vkResetCommandPool                         vkResetCommandPool                         ;
extern PFN_vkDestroyCommandPool                       vkDestroyCommandPool                       ;
extern PFN_vkBeginCommandBuffer                       vkBeginCommandBuffer                       ;
extern PFN_vkEndCommandBuffer                         vkEndCommandBuffer                         ;
extern PFN_vkAllocateCommandBuffers                   vkAllocateCommandBuffers                   ;
extern PFN_vkFreeCommandBuffers                       vkFreeCommandBuffers                       ;
extern PFN_vkCmdBeginRenderPass                       vkCmdBeginRenderPass                       ;
extern PFN_vkCmdCopyBufferToImage                     vkCmdCopyBufferToImage                     ;
extern PFN_vkCmdBlitImage                             vkCmdBlitImage                             ;
extern PFN_vkCmdNextSubpass                           vkCmdNextSubpass                           ;
extern PFN_vkCmdEndRenderPass                         vkCmdEndRenderPass                         ;
extern PFN_vkCmdBindPipeline                          vkCmdBindPipeline                          ;
extern PFN_vkCmdSetViewport                           vkCmdSetViewport                           ;
extern PFN_vkCmdSetScissor                            vkCmdSetScissor                            ;
extern PFN_vkCmdBindVertexBuffers                     vkCmdBindVertexBuffers                     ;
extern PFN_vkCmdBindIndexBuffer                       vkCmdBindIndexBuffer                       ;
extern PFN_vkCmdBindDescriptorSets                    vkCmdBindDescriptorSets                    ;
extern PFN_vkCmdDrawIndexed                           vkCmdDrawIndexed                           ;
extern PFN_vkCmdClearAttachments                      vkCmdClearAttachments                      ;
extern PFN_vkCmdDraw                                  vkCmdDraw                                  ;
extern PFN_vkCmdDispatch                              vkCmdDispatch                              ;
extern PFN_vkCmdCopyBuffer                            vkCmdCopyBuffer                            ;
extern PFN_vkCmdBeginDebugUtilsLabelEXT               vkCmdBeginDebugUtilsLabelEXT               ;
extern PFN_vkCmdEndDebugUtilsLabelEXT                 vkCmdEndDebugUtilsLabelEXT                 ;
extern PFN_vkQueueSubmit                              vkQueueSubmit                              ;
extern PFN_vkQueuePresentKHR                          vkQueuePresentKHR                          ;
extern PFN_vkQueueWaitIdle                            vkQueueWaitIdle                            ;
extern PFN_vkCreateRenderPass                         vkCreateRenderPass                         ;
extern PFN_vkDestroyRenderPass                        vkDestroyRenderPass                        ;
extern PFN_vkCreateBuffer                             vkCreateBuffer                             ;
extern PFN_vkDestroyBuffer                            vkDestroyBuffer                            ;
extern PFN_vkCreateSampler                            vkCreateSampler                            ;
extern PFN_vkDestroySampler                           vkDestroySampler                           ;
extern PFN_vkCreateImage                              vkCreateImage                              ;
extern PFN_vkDestroyImage                             vkDestroyImage                             ;
extern PFN_vkCreateImageView                          vkCreateImageView                          ;
extern PFN_vkDestroyImageView                         vkDestroyImageView                         ;
extern PFN_vkCreateFramebuffer                        vkCreateFramebuffer                        ;
extern PFN_vkDestroyFramebuffer                       vkDestroyFramebuffer                       ;
extern PFN_vkCmdPipelineBarrier                       vkCmdPipelineBarrier                       ;
extern PFN_vkCreateDescriptorPool                     vkCreateDescriptorPool                     ;
extern PFN_vkDestroyDescriptorPool                    vkDestroyDescriptorPool                    ;
extern PFN_vkAllocateDescriptorSets                   vkAllocateDescriptorSets                   ;
extern PFN_vkUpdateDescriptorSets                     vkUpdateDescriptorSets                     ;
extern PFN_vkFreeDescriptorSets                       vkFreeDescriptorSets                       ;
extern PFN_vkCreateDescriptorSetLayout                vkCreateDescriptorSetLayout                ;
extern PFN_vkDestroyDescriptorSetLayout               vkDestroyDescriptorSetLayout               ;
extern PFN_vkCreateSemaphore                          vkCreateSemaphore                          ;
extern PFN_vkDestroySemaphore                         vkDestroySemaphore                         ;
extern PFN_vkCreateSwapchainKHR                       vkCreateSwapchainKHR                       ;
extern PFN_vkGetSwapchainImagesKHR                    vkGetSwapchainImagesKHR                    ;
extern PFN_vkDestroySwapchainKHR                      vkDestroySwapchainKHR                      ;
extern PFN_vkDestroyDevice                            vkDestroyDevice                            ;
extern PFN_vkAcquireNextImageKHR                      vkAcquireNextImageKHR                      ;
extern PFN_vkCreateComputePipelines                   vkCreateComputePipelines                   ;
extern PFN_vkCreateGraphicsPipelines                  vkCreateGraphicsPipelines                  ;
extern PFN_vkCreatePipelineLayout                     vkCreatePipelineLayout                     ;
extern PFN_vkDestroyPipelineLayout                    vkDestroyPipelineLayout                    ;
extern PFN_vkDestroyPipeline                          vkDestroyPipeline                          ;
extern PFN_vkCreateShaderModule                       vkCreateShaderModule                       ;
extern PFN_vkDestroyShaderModule                      vkDestroyShaderModule                      ;



bool InitializeInstanceProcAddr();
void LoadInstanceFunctions(VkInstance instance);
void LoadDeviceFunctions(VkDevice device);