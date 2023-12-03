#pragma once
#include <render/types.h>

#include <utils/ILogListener.h>
#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        class CommandBuffer;
        class Pipeline;
    };

    namespace core {
        class FrameManager;
        class FrameContext : public utils::IWithLogging {
            public:
                vulkan::CommandBuffer* getCommandBuffer() const;
                vulkan::Pipeline* getPipeline() const;
                u32 getSwapChainImageIndex() const;
                bool begin();
                bool end();

            protected:
                friend class FrameManager;
                FrameContext();
                ~FrameContext();

                bool init(vulkan::LogicalDevice* device);
                void shutdown();
                void onAcquire(vulkan::CommandBuffer* buf, vulkan::Pipeline* pipeline);
                void onFree();

                vulkan::LogicalDevice* m_device;
                vulkan::Pipeline* m_pipeline;
                vulkan::CommandBuffer* m_buffer;
                VkSemaphore m_swapChainReady;
                VkSemaphore m_renderComplete;
                VkFence m_fence;
                bool m_frameStarted;
                u32 m_scImageIdx;
        };
    };
};