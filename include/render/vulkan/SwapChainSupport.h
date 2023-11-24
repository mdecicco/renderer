#pragma once
#include <render/types.h>

#include <utils/Array.h>
#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class PhysicalDevice;
        class Surface;

        class SwapChainSupport {
            public:
                SwapChainSupport();
                ~SwapChainSupport();

                bool isValid() const;
                bool hasFormat(VkFormat format, VkColorSpaceKHR colorSpace) const;
                bool hasPresentMode(VkPresentModeKHR mode) const;
                const VkSurfaceCapabilitiesKHR& getCapabilities() const;

            protected:
                friend class PhysicalDevice;

                const PhysicalDevice* m_device;
                const Surface* m_surface;
                VkSurfaceCapabilitiesKHR m_capabilities;
                utils::Array<VkSurfaceFormatKHR> m_formats;
                utils::Array<VkPresentModeKHR> m_presentModes;
        };
    };
};