#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/Surface.h>
#include <render/vulkan/QueueFamily.h>
#include <render/vulkan/SwapChainSupport.h>

#include <utils/Array.hpp>

namespace render {
    namespace vulkan {
        PhysicalDevice::PhysicalDevice() {
            m_instance = nullptr;
            m_handle = VK_NULL_HANDLE;
            m_props = {};
            m_features = {};
        }

        PhysicalDevice::PhysicalDevice(const PhysicalDevice& dev) {
            m_instance = dev.m_instance;
            m_handle = dev.m_handle;
            m_props = dev.m_props;
            m_memoryProps = dev.m_memoryProps;
            m_features = dev.m_features;
            m_availableExtensions = dev.m_availableExtensions;
            m_availableLayers = dev.m_availableLayers;
        }

        PhysicalDevice::~PhysicalDevice() {
        }

        bool PhysicalDevice::isDiscrete() const {
            return m_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        }

        bool PhysicalDevice::isVirtual() const {
            return m_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
        }

        bool PhysicalDevice::isIntegrated() const {
            return m_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
        }

        bool PhysicalDevice::isCPU() const {
            return m_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
        }

        bool PhysicalDevice::isExtensionAvailable(const char* name) const {
            constexpr size_t nameLen = sizeof(VkExtensionProperties::extensionName);
            for (u32 i = 0;i < m_availableExtensions.size();i++) {
                if (strncmp(name, m_availableExtensions[i].extensionName, nameLen) == 0) {
                    return true;
                }
            }

            return false;
        }

        bool PhysicalDevice::isLayerAvailable(const char* name) const {
            constexpr size_t nameLen = sizeof(VkLayerProperties::layerName);
            for (u32 i = 0;i < m_availableLayers.size();i++) {
                if (strncmp(name, m_availableLayers[i].layerName, nameLen) == 0) {
                    return true;
                }
            }

            return false;
        }

        bool PhysicalDevice::canPresentToSurface(Surface* surface, const QueueFamily& queueFamily) const {
            if (!m_handle) return false;

            VkBool32 canPresent = VK_FALSE;

            if (vkGetPhysicalDeviceSurfaceSupportKHR(m_handle, queueFamily.getIndex(), surface->get(), &canPresent) != VK_SUCCESS) {
                return false;
            }

            return canPresent == VK_TRUE;
        }

        bool PhysicalDevice::getSurfaceSwapChainSupport(Surface* surface, SwapChainSupport* out) const {
            if (!surface || !out || !m_handle) return false;

            if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_handle, surface->get(), &out->m_capabilities) != VK_SUCCESS) {
                return false;
            }

            u32 count;
            
            count = 0;
            if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_handle, surface->get(), &count, nullptr) != VK_SUCCESS) {
                out->m_capabilities = {};
                return false;
            }

            out->m_formats.reserve(count, true);
            if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_handle, surface->get(), &count, out->m_formats.data()) != VK_SUCCESS) {
                out->m_capabilities = {};
                out->m_formats.clear();
                return false;
            }
            
            count = 0;
            if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_handle, surface->get(), &count, nullptr) != VK_SUCCESS) {
                out->m_capabilities = {};
                out->m_formats.clear();
                return false;
            }

            out->m_presentModes.reserve(count, true);
            if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_handle, surface->get(), &count, out->m_presentModes.data()) != VK_SUCCESS) {
                out->m_capabilities = {};
                out->m_formats.clear();
                out->m_presentModes.clear();
                return false;
            }

            out->m_device = this;
            out->m_surface = surface;

            return true;
        }

        VkPhysicalDevice PhysicalDevice::get() const {
            return m_handle;
        }

        const VkPhysicalDeviceProperties& PhysicalDevice::getProperties() const {
            return m_props;
        }
        
        const VkPhysicalDeviceMemoryProperties& PhysicalDevice::getMemoryProperties() const {
            return m_memoryProps;
        }

        const VkPhysicalDeviceFeatures& PhysicalDevice::getFeatures() const {
            return m_features;
        }
        
        Instance* PhysicalDevice::getInstance() const {
            return m_instance;
        }

        utils::Array<PhysicalDevice> PhysicalDevice::list(Instance* instance) {
            if (!instance->isInitialized()) return {};

            u32 count = 0;
            if (vkEnumeratePhysicalDevices(instance->get(), &count, nullptr) != VK_SUCCESS) {
                return {};
            }

            if (count == 0) return {};
            VkPhysicalDevice* devices = new VkPhysicalDevice[count];
            if (vkEnumeratePhysicalDevices(instance->get(), &count, devices) != VK_SUCCESS) {
                delete [] devices;
                return {};
            }

            utils::Array<PhysicalDevice> out(count);
            for (u32 i = 0;i < count;i++) {
                out.emplace();
                PhysicalDevice& dev = out.last();
                dev.m_handle = devices[i];
                dev.m_instance = instance;
                vkGetPhysicalDeviceProperties(devices[i], &dev.m_props);
                vkGetPhysicalDeviceFeatures(devices[i], &dev.m_features);
                vkGetPhysicalDeviceMemoryProperties(devices[i], &dev.m_memoryProps);

                u32 extCount = 0;
                if (vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extCount, nullptr) != VK_SUCCESS) {
                    out.clear();
                    delete [] devices;
                    return {};
                }

                dev.m_availableExtensions.reserve(extCount, true);
                if (vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extCount, dev.m_availableExtensions.data()) != VK_SUCCESS) {
                    out.clear();
                    delete [] devices;
                    return {};
                }

                u32 layerCount = 0;
                if (vkEnumerateDeviceLayerProperties(devices[i], &layerCount, nullptr) != VK_SUCCESS) {
                    out.clear();
                    delete [] devices;
                    return {};
                }

                dev.m_availableLayers.reserve(extCount, true);
                if (vkEnumerateDeviceLayerProperties(devices[i], &layerCount, dev.m_availableLayers.data()) != VK_SUCCESS) {
                    out.clear();
                    delete [] devices;
                    return {};
                }
            }

            delete [] devices;

            return out;
        }
    };
};