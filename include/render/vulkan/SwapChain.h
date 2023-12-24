#pragma once
#include <render/types.h>

#include <utils/Array.h>
#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class Surface;
        class SwapChainSupport;
        class LogicalDevice;
        class GraphicsPipeline;
        class Texture;

        class SwapChain {
            public:
                SwapChain();
                ~SwapChain();

                VkSwapchainKHR get() const;
                LogicalDevice* getDevice() const;
                bool isValid() const;
                u32 getImageCount() const;
                const Array<VkImage>& getImages() const;
                const Array<VkImageView>& getImageViews() const;
                const Array<Texture*>& getDepthBuffers() const;
                const VkExtent2D& getExtent() const;
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
                bool recreate();
                void shutdown();
            
            protected:
                friend class GraphicsPipeline;
                friend class RenderPass;
                
                void onPipelineCreated(GraphicsPipeline* pipeline);
                void onPipelineDestroyed(GraphicsPipeline* pipeline);

                Surface* m_surface;
                LogicalDevice* m_device;
                VkSwapchainCreateInfoKHR m_createInfo;
                VkSwapchainKHR m_swapChain;
                VkFormat m_format;
                VkExtent2D m_extent;
                Array<VkImage> m_images;
                Array<VkImageView> m_imageViews;
                Array<Texture*> m_depthBuffers;
                Array<GraphicsPipeline*> m_pipelines;
        };
    };
};