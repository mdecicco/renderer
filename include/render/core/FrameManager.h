#pragma once
#include <render/types.h>

#include <utils/ILogListener.h>
#include <utils/List.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        class CommandPool;
        class CommandBuffer;
        class SwapChain;
        class RenderPass;
        class Framebuffer;
    };

    namespace core {
        class FrameContext;

        class FrameManager : public ::utils::IWithLogging {
            public:
                FrameManager(vulkan::RenderPass* renderPass, u32 maxLiveFrames);
                ~FrameManager();

                vulkan::CommandPool* getCommandPool() const;

                bool init();
                void shutdown();

                FrameContext* getFrame();
                void releaseFrame(FrameContext* frame);

            private:
                friend class FrameContext;

                vulkan::RenderPass* m_renderPass;
                vulkan::SwapChain* m_swapChain;
                vulkan::LogicalDevice* m_device;
                vulkan::CommandPool* m_cmdPool;
                Array<vulkan::Framebuffer*> m_framebuffers;

                struct FrameNode {
                    FrameContext* frame;
                    FrameNode* next;
                    FrameNode* last;
                };

                FrameNode* m_freeFrames;
                FrameNode* m_liveFrames;
                FrameNode* m_frames;
                u32 m_maxLiveFrames;
        };
    };
};