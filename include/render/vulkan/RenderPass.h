#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace core {
        class FrameManager;
        class FrameContext;
    };

    namespace vulkan {
        class LogicalDevice;
        class SwapChain;

        class RenderPass {
            public:
                RenderPass(SwapChain* swapChain);
                ~RenderPass();

                SwapChain* getSwapChain() const;
                core::FrameManager* getFrameManager() const;
                VkRenderPass get() const;

                bool init();
                bool recreate();
                void shutdown();

                core::FrameContext* getFrame();
                void releaseFrame(core::FrameContext* frame);
            
            protected:
                SwapChain* m_swapChain;
                core::FrameManager* m_frames;

                VkRenderPass m_renderPass;
        };
    };
};