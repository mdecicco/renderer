#include <render/vulkan/QueueFamily.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/Instance.h>

#include <utils/Array.hpp>

namespace render {
    namespace vulkan {
        QueueFamily::QueueFamily() {
            m_props = {};
            m_device = nullptr;
            m_index = -1;
        }
        
        QueueFamily::QueueFamily(const QueueFamily& qf) {
            m_props = qf.m_props;
            m_device = qf.m_device;
            m_index = qf.m_index;
        }

        QueueFamily::~QueueFamily() {
        }

        bool QueueFamily::supportsGraphics() const {
            return m_props.queueFlags & VK_QUEUE_GRAPHICS_BIT;
        }

        bool QueueFamily::supportsCompute() const {
            return m_props.queueFlags & VK_QUEUE_COMPUTE_BIT;
        }

        bool QueueFamily::supportsTransfer() const {
            return m_props.queueFlags & VK_QUEUE_TRANSFER_BIT;
        }

        const VkQueueFamilyProperties& QueueFamily::getProperties() const {
            return m_props;
        }

        PhysicalDevice* QueueFamily::getDevice() const {
            return m_device;
        }

        Instance* QueueFamily::getInstance() const {
            if (!m_device) return nullptr;
            return m_device->getInstance();
        }

        i32 QueueFamily::getIndex() const {
            return m_index;
        }

        Array<QueueFamily> QueueFamily::list(PhysicalDevice* device) {
            u32 count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device->get(), &count, nullptr);

            if (count == 0) return {};
            VkQueueFamilyProperties* families = new VkQueueFamilyProperties[count];
            vkGetPhysicalDeviceQueueFamilyProperties(device->get(), &count, families);

            Array<QueueFamily> out(count);
            for (u32 i = 0;i < count;i++) {
                out.emplace();
                QueueFamily& fam = out.last();
                fam.m_props = families[i];
                fam.m_device = device;
                fam.m_index = i;
            }

            delete [] families;

            return out;
        }
    };
};