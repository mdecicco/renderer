#include <render/vulkan/Queue.h>
#include <render/vulkan/QueueFamily.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/CommandBuffer.h>

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

        bool Queue::submit(
            CommandBuffer* buffer,
            VkFence fence,
            u32 waitForCount,
            VkSemaphore* waitFor,
            u32 signalCount,
            VkSemaphore* signal,
            VkPipelineStageFlags waitStageMask
        ) const {
            VkCommandBuffer cb = buffer->get();

            VkSubmitInfo si = {};
            si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.commandBufferCount = 1;
            si.pCommandBuffers = &cb;
            si.waitSemaphoreCount = waitForCount;
            si.pWaitSemaphores = waitFor;
            si.signalSemaphoreCount = signalCount;
            si.pSignalSemaphores = signal;

            if (waitStageMask != VK_PIPELINE_STAGE_NONE) {
                si.pWaitDstStageMask = &waitStageMask;
            }

            return vkQueueSubmit(m_queue, 1, &si, fence) == VK_SUCCESS;
        }
        
        bool Queue::waitForIdle() const {
            return vkQueueWaitIdle(m_queue) == VK_SUCCESS;
        }
    };
};