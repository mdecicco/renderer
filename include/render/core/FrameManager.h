#pragma once
#include <render/types.h>

#include <utils/ILogListener.h>
#include <utils/List.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        class CommandPool;
        class CommandBuffer;
        class Pipeline;
    };

    namespace core {
        class FrameContext;

        class FrameManager : public utils::IWithLogging {
            public:
                FrameManager(vulkan::LogicalDevice* device, u32 maxLiveFrames);
                ~FrameManager();

                bool init();
                void shutdown();

                FrameContext* getFrame(vulkan::Pipeline* pipeline);
                void releaseFrame(FrameContext* frame);

            protected:
                vulkan::LogicalDevice* m_device;
                vulkan::CommandPool* m_cmdPool;

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