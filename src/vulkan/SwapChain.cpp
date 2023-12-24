#include <render/vulkan/SwapChain.h>
#include <render/vulkan/Surface.h>
#include <render/vulkan/SwapChainSupport.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/Queue.h>
#include <render/vulkan/QueueFamily.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/GraphicsPipeline.h>
#include <render/vulkan/RenderPass.h>
#include <render/vulkan/Texture.h>
#include <render/core/FrameManager.h>

#include <utils/Window.h>
#include <utils/Array.hpp>

namespace render {
    namespace vulkan {
        SwapChain::SwapChain() {
            m_surface = nullptr;
            m_device = nullptr;
            m_swapChain = VK_NULL_HANDLE;
            m_format = VK_FORMAT_UNDEFINED;
        }

        SwapChain::~SwapChain() {
            shutdown();
        }

        VkSwapchainKHR SwapChain::get() const {
            return m_swapChain;
        }

        LogicalDevice* SwapChain::getDevice() const {
            return m_device;
        }

        bool SwapChain::isValid() const {
            return m_swapChain != nullptr;
        }

        u32 SwapChain::getImageCount() const {
            return m_images.size();
        }
        
        const Array<VkImage>& SwapChain::getImages() const {
            return m_images;
        }
        
        const Array<VkImageView>& SwapChain::getImageViews() const {
            return m_imageViews;
        }
        
        const Array<Texture*>& SwapChain::getDepthBuffers() const {
            return m_depthBuffers;
        }
        
        const VkExtent2D& SwapChain::getExtent() const {
            return m_extent;
        }
        
        VkFormat SwapChain::getFormat() const {
            return m_format;
        }

        bool SwapChain::init(
            Surface* surface,
            LogicalDevice* device,
            const SwapChainSupport& support,
            VkFormat format,
            VkColorSpaceKHR colorSpace,
            VkPresentModeKHR presentMode,
            u32 imageCount,
            VkImageUsageFlags usage,
            VkCompositeAlphaFlagBitsKHR compositeAlpha,
            SwapChain* previous
        ) {
            if (m_swapChain) return false;

            m_surface = surface;
            m_device = device;

            auto capabilities = support.getCapabilities();

            if (capabilities.currentExtent.width != UINT32_MAX) {
                m_extent = capabilities.currentExtent;
            } else {
                surface->getWindow()->getSize(
                    &m_extent.width,
                    &m_extent.height
                );
            }

            m_createInfo = {};
            m_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            m_createInfo.surface = m_surface->get();
            m_createInfo.imageFormat = format;
            m_createInfo.imageColorSpace = colorSpace;
            m_createInfo.presentMode = presentMode;
            m_createInfo.minImageCount = imageCount;
            m_createInfo.imageUsage = usage;
            m_createInfo.compositeAlpha = compositeAlpha;
            m_createInfo.imageArrayLayers = 1;
            m_createInfo.clipped = VK_TRUE;
            m_createInfo.oldSwapchain = previous ? previous->get() : VK_NULL_HANDLE;
            m_createInfo.imageExtent = m_extent;
            m_createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

            auto presentQueue = device->getPresentationQueue();
            auto gfxQueue = device->getGraphicsQueue();

            if (!presentQueue || !gfxQueue) return false;
            u32 queueFamilyIndices[] = {
                u32(gfxQueue->getFamily().getIndex()),
                u32(presentQueue->getFamily().getIndex())
            };

            if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
                m_createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                m_createInfo.queueFamilyIndexCount = 2;
                m_createInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                m_createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                m_createInfo.queueFamilyIndexCount = 0;
                m_createInfo.pQueueFamilyIndices = nullptr;
            }

            if (vkCreateSwapchainKHR(device->get(), &m_createInfo, device->getInstance()->getAllocator(), &m_swapChain) != VK_SUCCESS) {
                return false;
            }

            u32 count = 0;
            if (vkGetSwapchainImagesKHR(device->get(), m_swapChain, &count, nullptr) != VK_SUCCESS) {
                shutdown();
                return false;
            }

            m_images.reserve(count, true);
            if (vkGetSwapchainImagesKHR(device->get(), m_swapChain, &count, m_images.data()) != VK_SUCCESS) {
                m_images.clear();
                shutdown();
                return false;
            }

            m_imageViews.reserve(count);
            m_depthBuffers.reserve(count);
            for (u32 i = 0;i < count;i++) {
                VkImageViewCreateInfo iv = {};
                iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                iv.image = m_images[i];
                iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
                iv.format = format;
                iv.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                iv.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                iv.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                iv.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                iv.subresourceRange.layerCount = 1;
                iv.subresourceRange.levelCount = 1;
                iv.subresourceRange.baseMipLevel = 0;
                iv.subresourceRange.baseArrayLayer = 0;
                iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

                VkImageView view = VK_NULL_HANDLE;
                if (vkCreateImageView(device->get(), &iv, device->getInstance()->getAllocator(), &view) != VK_SUCCESS) {
                    shutdown();
                    return false;
                }
                m_imageViews.push(view);

                Texture* depth = new Texture(m_device);
                bool r = depth->init(
                    m_extent.width,
                    m_extent.height,
                    VK_FORMAT_D32_SFLOAT,
                    VK_IMAGE_TYPE_2D,
                    1, 1, 1,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED
                );

                if (!r) {
                    delete depth;
                    shutdown();
                    return false;
                }

                m_depthBuffers.push(depth);
            }

            m_format = format;

            return true;
        }

        bool SwapChain::recreate() {
            if (!m_swapChain) return false;

            auto prevViews = m_imageViews;
            m_imageViews.clear();
            m_images.clear();

            // Create new swapchain
            SwapChainSupport support;
            m_device->getPhysicalDevice()->getSurfaceSwapChainSupport(m_surface, &support);

            auto capabilities = support.getCapabilities();
            if (capabilities.currentExtent.width != UINT32_MAX) {
                m_extent = capabilities.currentExtent;
            } else {
                m_surface->getWindow()->getSize(
                    &m_extent.width,
                    &m_extent.height
                );
            }

            m_createInfo.oldSwapchain = m_swapChain;
            m_createInfo.imageExtent = m_extent;

            auto presentQueue = m_device->getPresentationQueue();
            auto gfxQueue = m_device->getGraphicsQueue();

            if (!presentQueue || !gfxQueue) return false;
            u32 queueFamilyIndices[] = {
                u32(gfxQueue->getFamily().getIndex()),
                u32(presentQueue->getFamily().getIndex())
            };

            if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
                m_createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                m_createInfo.queueFamilyIndexCount = 2;
                m_createInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                m_createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                m_createInfo.queueFamilyIndexCount = 0;
                m_createInfo.pQueueFamilyIndices = nullptr;
            }

            VkSwapchainKHR newSwapChain = VK_NULL_HANDLE;
            if (vkCreateSwapchainKHR(m_device->get(), &m_createInfo, m_device->getInstance()->getAllocator(), &newSwapChain) != VK_SUCCESS) {
                return false;
            }

            u32 count = 0;
            if (vkGetSwapchainImagesKHR(m_device->get(), newSwapChain, &count, nullptr) != VK_SUCCESS) {
                shutdown();
                return false;
            }

            m_images.reserve(count, true);
            if (vkGetSwapchainImagesKHR(m_device->get(), newSwapChain, &count, m_images.data()) != VK_SUCCESS) {
                m_images.clear();
                shutdown();
                return false;
            }

            m_imageViews.reserve(count);
            for (u32 i = 0;i < count;i++) {
                VkImageViewCreateInfo iv = {};
                iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                iv.image = m_images[i];
                iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
                iv.format = m_createInfo.imageFormat;
                iv.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                iv.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                iv.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                iv.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                iv.subresourceRange.layerCount = 1;
                iv.subresourceRange.levelCount = 1;
                iv.subresourceRange.baseMipLevel = 0;
                iv.subresourceRange.baseArrayLayer = 0;
                iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

                VkImageView view = VK_NULL_HANDLE;
                if (vkCreateImageView(m_device->get(), &iv, m_device->getInstance()->getAllocator(), &view) != VK_SUCCESS) {
                    shutdown();
                    return false;
                }
                m_imageViews.push(view);

                // reuse old depth buffer Texture objects
                m_depthBuffers[i]->shutdown();
                bool r = m_depthBuffers[i]->init(
                    m_extent.width,
                    m_extent.height,
                    VK_FORMAT_D32_SFLOAT,
                    VK_IMAGE_TYPE_2D,
                    1, 1, 1,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED
                );

                if (!r) {
                    shutdown();
                    return false;
                }
            }

            // Destroy old image views
            for (u32 i = 0;i < prevViews.size();i++) {
                vkDestroyImageView(m_device->get(), prevViews[i], m_device->getInstance()->getAllocator());
            }

            // Destroy old swapchain
            vkDestroySwapchainKHR(m_device->get(), m_swapChain, m_device->getInstance()->getAllocator());

            m_swapChain = newSwapChain;
            
            // Update pipelines
            for (u32 i = 0;i < m_pipelines.size();i++) {
                if (!m_pipelines[i]->recreate()) {
                    shutdown();
                    return false;
                }
            }

            return true;
        }

        void SwapChain::shutdown() {
            if (!m_swapChain) return;

            for (u32 i = 0;i < m_imageViews.size();i++) {
                vkDestroyImageView(m_device->get(), m_imageViews[i], m_device->getInstance()->getAllocator());
            }

            for (u32 i = 0;i < m_depthBuffers.size();i++) {
                delete m_depthBuffers[i];
            }

            vkDestroySwapchainKHR(m_device->get(), m_swapChain, m_device->getInstance()->getAllocator());
            m_swapChain = VK_NULL_HANDLE;
            m_device = nullptr;
            m_surface = nullptr;
            m_imageViews.clear();
            m_images.clear();
            m_depthBuffers.clear();
            m_format = VK_FORMAT_UNDEFINED;
            m_createInfo = {};
        }
        
        void SwapChain::onPipelineCreated(GraphicsPipeline* pipeline) {
            m_pipelines.push(pipeline);
        }

        void SwapChain::onPipelineDestroyed(GraphicsPipeline* pipeline) {
            i64 idx = m_pipelines.findIndex([pipeline](GraphicsPipeline* p) {
                return p == pipeline;
            });
            if (idx == -1) return;
            m_pipelines.remove(u32(idx));
        }
    };
};