#pragma once
#include <render/types.h>
#include <render/vulkan/QueueFamily.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        class Queue {
            public:
                LogicalDevice* getDevice() const;
                const QueueFamily& getFamily() const;
                u32 getIndex() const;

                bool supportsGraphics() const;
                bool supportsCompute() const;
                bool supportsTransfer() const;

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