#pragma once
#include <render/types.h>

#include <utils/Array.h>
#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class Surface;
        class SwapChainSupport;
        class LogicalDevice;

        class SwapChain {
            public:
                SwapChain();
                ~SwapChain();

                VkSwapchainKHR get() const;
                bool isValid() const;
                const utils::Array<VkImage>& getImages() const;
                const utils::Array<VkImageView>& getImageViews() const;
                VkFormat getFormat() const;

                bool init(
                    Surface* surface,
                    LogicalDevice* device,
                    const SwapChainSupport& support,
                    VkFormat format,
                    VkColorSpaceKHR colorSpace,
                    VkPresentModeKHR presentMode,
                    u32 imageCount,
                    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                    SwapChain* previous = nullptr
                );
                void shutdown();
            
            protected:
                Surface* m_surface;
                LogicalDevice* m_device;
                VkSwapchainKHR m_swapChain;
                VkFormat m_format;
                utils::Array<VkImage> m_images;
                utils::Array<VkImageView> m_imageViews;
        };
    };
};