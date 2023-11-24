#include <render/vulkan/SwapChain.h>
#include <render/vulkan/Surface.h>
#include <render/vulkan/SwapChainSupport.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Queue.h>
#include <render/vulkan/QueueFamily.h>
#include <render/vulkan/Instance.h>

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

        bool SwapChain::isValid() const {
            return m_swapChain != nullptr;
        }
        
        const utils::Array<VkImage>& SwapChain::getImages() const {
            return m_images;
        }
        
        const utils::Array<VkImageView>& SwapChain::getImageViews() const {
            return m_imageViews;
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

            VkSwapchainCreateInfoKHR ci = {};
            ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            ci.surface = m_surface->get();
            ci.imageFormat = format;
            ci.imageColorSpace = colorSpace;
            ci.presentMode = presentMode;
            ci.minImageCount = imageCount;
            ci.imageUsage = usage;
            ci.compositeAlpha = compositeAlpha;
            ci.imageArrayLayers = 1;
            ci.clipped = VK_TRUE;
            ci.oldSwapchain = previous ? previous->get() : VK_NULL_HANDLE;

            if (capabilities.currentExtent.width != UINT32_MAX) {
                ci.imageExtent = capabilities.currentExtent;
            } else {
                surface->getWindow()->getSize(
                    &ci.imageExtent.width,
                    &ci.imageExtent.height
                );
            }

            auto presentQueue = device->getPresentationQueue();
            auto gfxQueue = device->getGraphicsQueue();

            if (!presentQueue || !gfxQueue) return false;
            u32 queueFamilyIndices[] = {
                u32(gfxQueue->getFamily().getIndex()),
                u32(presentQueue->getFamily().getIndex())
            };

            if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
                ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                ci.queueFamilyIndexCount = 2;
                ci.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                ci.queueFamilyIndexCount = 0;
                ci.pQueueFamilyIndices = nullptr;
            }

            if (vkCreateSwapchainKHR(device->get(), &ci, device->getInstance()->getAllocator(), &m_swapChain) != VK_SUCCESS) {
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
            }

            m_format = format;

            return true;
        }

        void SwapChain::shutdown() {
            if (!m_swapChain) return;

            for (u32 i = 0;i < m_imageViews.size();i++) {
                vkDestroyImageView(m_device->get(), m_imageViews[i], m_device->getInstance()->getAllocator());
            }

            vkDestroySwapchainKHR(m_device->get(), m_swapChain, m_device->getInstance()->getAllocator());
            m_swapChain = nullptr;
            m_device = nullptr;
            m_surface = nullptr;
            m_imageViews.clear();
            m_images.clear();
            m_format = VK_FORMAT_UNDEFINED;
        }
    };
};