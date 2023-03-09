#pragma once
#include <vulkan/vulkan.h>

#define GET_DEVICE_FUNC(x) _##x = (PFN_##x)vkGetDeviceProcAddr(device, #x)

// function pointers
extern PFN_vkWaitForFences              _vkWaitForFences              ;
extern PFN_vkResetFences                _vkResetFences                ;
extern PFN_vkResetCommandPool           _vkResetCommandPool           ;
extern PFN_vkBeginCommandBuffer         _vkBeginCommandBuffer         ;
extern PFN_vkEndCommandBuffer           _vkEndCommandBuffer           ;
extern PFN_vkCmdBeginRenderPass         _vkCmdBeginRenderPass         ;
extern PFN_vkCmdCopyBufferToImage       _vkCmdCopyBufferToImage       ;
extern PFN_vkCmdBlitImage               _vkCmdBlitImage               ;
extern PFN_vkCmdNextSubpass             _vkCmdNextSubpass             ;
extern PFN_vkCmdEndRenderPass           _vkCmdEndRenderPass           ;
extern PFN_vkCmdBindPipeline            _vkCmdBindPipeline            ;
extern PFN_vkCmdSetViewport             _vkCmdSetViewport             ;
extern PFN_vkCmdSetScissor              _vkCmdSetScissor              ;
extern PFN_vkCmdBindVertexBuffers       _vkCmdBindVertexBuffers       ;
extern PFN_vkCmdBindIndexBuffer         _vkCmdBindIndexBuffer         ;
extern PFN_vkCmdBindDescriptorSets      _vkCmdBindDescriptorSets      ;
extern PFN_vkCmdDrawIndexed             _vkCmdDrawIndexed             ;
extern PFN_vkCmdClearAttachments        _vkCmdClearAttachments        ;
extern PFN_vkCmdDraw                    _vkCmdDraw                    ;
extern PFN_vkCmdDispatch                _vkCmdDispatch                ;
extern PFN_vkCmdCopyBuffer              _vkCmdCopyBuffer              ;
extern PFN_vkCmdBeginDebugUtilsLabelEXT _vkCmdBeginDebugUtilsLabelEXT ;
extern PFN_vkCmdEndDebugUtilsLabelEXT   _vkCmdEndDebugUtilsLabelEXT   ;