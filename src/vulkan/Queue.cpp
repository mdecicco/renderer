#include <render/vulkan/Queue.h>
#include <render/vulkan/QueueFamily.h>
#include <render/vulkan/LogicalDevice.h>

namespace render {
    namespace vulkan {
        Queue::Queue(LogicalDevice* device, const QueueFamily& family, u32 queueIndex) : m_family(family) {
            m_device = device;
            m_queueIndex = queueIndex;

            vkGetDeviceQueue(device->get(), family.getIndex(), queueIndex, &m_queue);
        }

        Queue::~Queue() {
        }

        VkQueue Queue::get() const {
            return m_queue;
        }

        LogicalDevice* Queue::getDevice() const {
            return m_device;
        }

        const QueueFamily& Queue::getFamily() const {
            return m_family;
        }
        
        u32 Queue::getIndex() const {
            return m_queueIndex;
        }

        bool Queue::supportsGraphics() const {
            return m_family.supportsGraphics();
        }

        bool Queue::supportsCompute() const {
            return m_family.supportsCompute();
        }

        bool Queue::supportsTransfer() const {
            return m_family.supportsTransfer();
        }
    };
};