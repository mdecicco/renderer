#include <render/core/FrameManager.h>
#include <render/core/FrameContext.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/CommandPool.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/RenderPass.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/Texture.h>
#include <render/vulkan/Queue.h>
#include <render/vulkan/Framebuffer.h>

#include <utils/Array.hpp>

namespace render {
    namespace core {
        FrameManager::FrameManager(vulkan::SwapChain* swapChain, vulkan::RenderPass* renderPass) : utils::IWithLogging("Frame Manager") {
            m_renderPass = renderPass;
            m_swapChain = swapChain;
            m_device = m_swapChain->getDevice();
            m_cmdPool = new vulkan::CommandPool(m_device, &m_device->getGraphicsQueue()->getFamily());
            m_frameCount = m_swapChain->getImageCount();

            m_frames = new FrameNode[m_frameCount];
            m_liveFrames = nullptr;
            m_freeFrames = m_frames;
            for (u32 i = 0;i < m_frameCount;i++) {
                m_frames[i].frame = new FrameContext();
                m_frames[i].frame->subscribeLogger(this);
                m_frames[i].frame->m_mgr = this;

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
            shutdown();
            
            delete m_cmdPool;
            m_cmdPool = nullptr;
            
            for (u32 i = 0;i < m_frameCount;i++) {
                delete m_frames[i].frame;
            }

            delete [] m_frames;
            m_freeFrames = m_liveFrames = m_frames = nullptr;
        }

        vulkan::CommandPool* FrameManager::getCommandPool() const {
            return m_cmdPool;
        }
        
        u32 FrameManager::getFrameCount() const {
            return m_frameCount;
        }

        bool FrameManager::init() {
            if (!m_cmdPool->init(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)) return false;

            m_framebuffers.reserve(m_frameCount);
            for (u32 i = 0;i < m_frameCount;i++) {
                vulkan::CommandBuffer* cb = m_cmdPool->createBuffer(true);
                if (!cb) {
                    fatal("Failed to acquire command buffer for frame");
                    shutdown();
                    return false;
                }

                if (!m_frames[i].frame->init(m_swapChain, cb)) return false;

                vulkan::Framebuffer* fb = new vulkan::Framebuffer(m_renderPass);
                fb->attach(m_swapChain->getImageViews()[i], m_swapChain->getFormat());
                fb->attach(m_swapChain->getDepthBuffers()[i]);

                auto& extent = m_swapChain->getExtent();
                if (!fb->init(vec2ui(extent.width, extent.height))) {
                    fatal("Failed to create framebuffer for frame");
                    shutdown();
                    return false;
                }

                m_framebuffers.push(fb);
            }

            return true;
        }

        void FrameManager::shutdown() {
            for (u32 i = 0;i < m_frameCount;i++) {
                m_frames[i].frame->shutdown();
                m_framebuffers[i]->shutdown();
            }

            m_framebuffers.clear();
            m_cmdPool->shutdown();
        }

        FrameContext* FrameManager::getFrame() {
            if (!m_freeFrames) return nullptr;

            FrameNode* n = m_freeFrames;
            if (n->next) n->next->last = nullptr;
            m_freeFrames = n->next;

            if (m_liveFrames) m_liveFrames->last = n;
            n->next = m_liveFrames;
            m_liveFrames = n;

            n->frame->onAcquire();

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

            n->frame->onFree();
        }
    };
};