#include <render/core/FrameManager.h>
#include <render/core/FrameContext.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/CommandPool.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/Queue.h>

namespace render {
    namespace core {
        FrameManager::FrameManager(vulkan::LogicalDevice* device, u32 maxLiveFrames) : utils::IWithLogging("Frame Manager") {
            m_device = device;
            m_cmdPool = new vulkan::CommandPool(device, &device->getGraphicsQueue()->getFamily());
            m_maxLiveFrames = maxLiveFrames;

            m_frames = new FrameNode[maxLiveFrames];
            m_liveFrames = nullptr;
            m_freeFrames = m_frames;
            for (u32 i = 0;i < maxLiveFrames;i++) {
                m_frames[i].frame = new FrameContext();
                m_frames[i].frame->subscribeLogger(this);
                if (i > 0) {
                    m_frames[i].last = &m_frames[i - 1];
                    m_frames[i].next = nullptr;
                    m_frames[i - 1].next = &m_frames[i];
                } else {
                    m_frames[i].last = nullptr;
                    m_frames[i].next = nullptr;
                }
            }
        }

        FrameManager::~FrameManager() {
            delete m_cmdPool;
            m_cmdPool = nullptr;
            
            for (u32 i = 0;i < m_maxLiveFrames;i++) {
                delete m_frames[i].frame;
            }

            delete [] m_frames;
            m_freeFrames = m_liveFrames = m_frames = nullptr;
        }

        bool FrameManager::init() {
            if (!m_cmdPool->init(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)) return false;

            for (u32 i = 0;i < m_maxLiveFrames;i++) {
                if (!m_frames[i].frame->init(m_device)) return false;
            }

            return true;
        }

        void FrameManager::shutdown() {
            for (u32 i = 0;i < m_maxLiveFrames;i++) {
                m_frames[i].frame->shutdown();
            }

            m_cmdPool->shutdown();
        }

        FrameContext* FrameManager::getFrame(vulkan::Pipeline* pipeline) {
            if (!m_freeFrames) return nullptr;

            FrameNode* n = m_freeFrames;
            if (n->next) n->next->last = nullptr;
            m_freeFrames = n->next;

            if (m_liveFrames) m_liveFrames->last = n;
            n->next = m_liveFrames;
            m_liveFrames = n;

            n->frame->onAcquire(m_cmdPool->createBuffer(true), pipeline);

            return n->frame;
        }

        void FrameManager::releaseFrame(FrameContext* frame) {
            FrameNode* n = m_liveFrames;
            while (n) {
                if (n->frame == frame) break;
                n = n->next;
            }

            if (!n) return;
            if (n->last) n->last->next = n->next;
            if (n->next) n->next-> last = n->last;

            if (m_freeFrames) m_freeFrames->last = n;
            n->next = m_freeFrames;
            m_freeFrames = n;

            m_cmdPool->freeBuffer(n->frame->m_buffer);
            n->frame->onFree();
        }
    };
};