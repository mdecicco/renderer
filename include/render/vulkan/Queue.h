#pragma once
#include <render/types.h>
#include <render/vulkan/QueueFamily.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        class CommandBuffer;

        class Queue {
            public:
                VkQueue get() const;
                LogicalDevice* getDevice() const;
                const QueueFamily& getFamily() const;
                u32 getIndex() const;

                bool supportsGraphics() const;
                bool supportsCompute() const;
                bool supportsTransfer() const;

                bool submit(
                    CommandBuffer* buffer,
                    VkFence fence = VK_NULL_HANDLE,
                    u32 waitForCount = 0,
                    VkSemaphore* waitFor = nullptr,
                    u32 signalCount = 0,
                    VkSemaphore* signal = nullptr,
                    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_NONE
                ) const;
                bool waitForIdle() const;

            protected:
                friend class LogicalDevice;
                Queue(LogicalDevice* device, const QueueFamily& family, u32 queueIndex);
                ~Queue();

                LogicalDevice* m_device;
                QueueFamily m_family;
                VkQueue m_queue;
                u32 m_queueIndex;
        };
    };
};