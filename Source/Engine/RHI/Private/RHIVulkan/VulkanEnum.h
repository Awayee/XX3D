#pragma once
#include <vulkan/vulkan.h>
#include "RHI/Public/RHIEnum.h"

VkFormat ConvertVkFormat(Engine::RFormat f);

VkImageLayout ConvertVkImageLayout(Engine::RImageLayout layout);
