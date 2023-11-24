#include <render/vulkan/SwapChainSupport.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/Surface.h>

#include <utils/Array.hpp>

namespace render {
    namespace vulkan {
        SwapChainSupport::SwapChainSupport() {
            m_device = nullptr;
            m_surface = nullptr;
        }

        SwapChainSupport::~SwapChainSupport() {
        }
        
        bool SwapChainSupport::isValid() const {
            return m_device && m_surface && m_formats.size() > 0 && m_presentModes.size() > 0;
        }

        bool SwapChainSupport::hasFormat(VkFormat format, VkColorSpaceKHR colorSpace) const {
            return m_formats.some([format, colorSpace](const VkSurfaceFormatKHR& fmt) {
                return fmt.format == format && fmt.colorSpace == colorSpace;
            });
        }

        bool SwapChainSupport::hasPresentMode(VkPresentModeKHR mode) const {
            return m_presentModes.some([mode](const VkPresentModeKHR& pmode) {
                return pmode == mode;
            });
        }
        
        const VkSurfaceCapabilitiesKHR& SwapChainSupport::getCapabilities() const {
            return m_capabilities;
        }
    };
};