#pragma once
#include <render/types.h>

#include <utils/Array.h>
#include <vulkan/vulkan.h>
namespace render {
    namespace vulkan {
        class LogicalDevice;
        class QueueFamily;
        class CommandBuffer;

        class CommandPool {
            public:
                CommandPool(LogicalDevice* device, const QueueFamily* family);
                ~CommandPool();

                bool init(VkCommandPoolCreateFlags flags = 0);
                void shutdown();

                VkCommandPool get() const;
                const QueueFamily* getFamily() const;
                VkCommandPoolCreateFlags getFlags() const;

                CommandBuffer* createBuffer(bool primary);
                void freeBuffer(CommandBuffer* buffer);

            protected:
                LogicalDevice* m_device;
                const QueueFamily* m_family;
                VkCommandPool m_pool;
                VkCommandPoolCreateFlags m_flags;
                utils::Array<CommandBuffer*> m_buffers;
        };
    };
};