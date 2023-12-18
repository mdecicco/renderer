#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        /*
         * Thanks to:
         * - Mark Lobodzinski <mark@lunarg.com>
         * - Dave Houlton <daveh@lunarg.com>
         * 
         * for this utility function
         * https://android.googlesource.com/platform/external/vulkan-validation-layers/+/HEAD/layers/vk_format_utils.cpp
         * 
         */

        struct VulkanFormatInfo {
            u8 size;
            u8 channelCount;
            u8 blockSize;
            bool isFloatingPoint;
            bool isSigned;
        };

        const VulkanFormatInfo& getFormatInfo(VkFormat fmt);
    };
};