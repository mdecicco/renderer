#pragma once
#include <render/types.h>

#include <utils/ILogListener.h>
#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        class CommandBuffer;
        class SwapChain;
        class Framebuffer;
        class RenderPass;
    };

    namespace core {
        class FrameManager;
        class FrameContext : public ::utils::IWithLogging {
            public:
                vulkan::CommandBuffer* getCommandBuffer() const;
                vulkan::SwapChain* getSwapChain() const;
                vulkan::Framebuffer* getFramebuffer() const;
                u32 getSwapChainImageIndex() const;
                void setClearColor(u32 attachmentIdx, const vec4f& clearColor);
                void setClearColor(u32 attachmentIdx, const vec4ui& clearColor);
                void setClearColor(u32 attachmentIdx, const vec4i& clearColor);
                void setClearDepthStencil(u32 attachmentIdx, f32 clearDepth, u32 clearStencil = 0);
                bool begin();
                bool end();

            private:
                friend class FrameManager;
                FrameContext();
                ~FrameContext();

                bool init(vulkan::SwapChain* swapChain, vulkan::CommandBuffer* cb);
                void shutdown();
                void onAcquire();
                void onFree();

                vulkan::LogicalDevice* m_device;
                vulkan::SwapChain* m_swapChain;
                vulkan::CommandBuffer* m_buffer;
                vulkan::Framebuffer* m_framebuffer;
                FrameManager* m_mgr;

                VkSemaphore m_swapChainReady;
                VkSemaphore m_renderComplete;
                VkFence m_fence;
                bool m_frameStarted;
                u32 m_scImageIdx;
        };
    };
};