#include <render/core/FrameContext.h>
#include <render/core/FrameManager.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/Pipeline.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/RenderPass.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/Framebuffer.h>
#include <render/vulkan/Queue.h>

namespace render {
    namespace core {
        FrameContext::FrameContext() : utils::IWithLogging("Frame") {
            m_swapChain = nullptr;
            m_buffer = nullptr;
            m_framebuffer = nullptr;
            m_swapChainReady = VK_NULL_HANDLE;
            m_renderComplete = VK_NULL_HANDLE;
            m_fence = VK_NULL_HANDLE;
            m_scImageIdx = 0;
            m_frameStarted = false;
        }

        FrameContext::~FrameContext() {
            shutdown();
        }

        vulkan::CommandBuffer* FrameContext::getCommandBuffer() const {
            return m_buffer;
        }

        vulkan::SwapChain* FrameContext::getSwapChain() const {
            return m_swapChain;
        }
        
        vulkan::Framebuffer* FrameContext::getFramebuffer() const {
            return m_framebuffer;
        }
        
        u32 FrameContext::getSwapChainImageIndex() const {
            return m_scImageIdx;
        }

        void FrameContext::setClearColor(u32 attachmentIdx, const vec4f& clearColor) {
            if (!m_framebuffer) return;
            m_framebuffer->setClearColor(attachmentIdx, clearColor);
        }

        void FrameContext::setClearColor(u32 attachmentIdx, const vec4ui& clearColor) {
            if (!m_framebuffer) return;
            m_framebuffer->setClearColor(attachmentIdx, clearColor);
        }

        void FrameContext::setClearColor(u32 attachmentIdx, const vec4i& clearColor) {
            if (!m_framebuffer) return;
            m_framebuffer->setClearColor(attachmentIdx, clearColor);
        }

        void FrameContext::setClearDepthStencil(u32 attachmentIdx, f32 clearDepth, u32 clearStencil) {
            if (!m_framebuffer) return;
            m_framebuffer->setClearDepthStencil(attachmentIdx, clearDepth, clearStencil);
        }

        bool FrameContext::begin() {
            if (m_frameStarted || !m_buffer) return false;

            if (vkWaitForFences(m_device->get(), 1, &m_fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) return false;
            if (vkResetFences(m_device->get(), 1, &m_fence) != VK_SUCCESS) return false;

            if (vkAcquireNextImageKHR(m_device->get(), m_swapChain->get(), UINT64_MAX, m_swapChainReady, nullptr, &m_scImageIdx) != VK_SUCCESS) {
                return false;
            }

            m_framebuffer = m_mgr->m_framebuffers[m_scImageIdx];

            if (!m_buffer->reset()) return false;
            if (!m_buffer->begin()) return false;
            m_frameStarted = true;

            return true;
        }

        bool FrameContext::end() {
            if (!m_frameStarted || !m_buffer->end()) return false;

            bool submitResult = m_device->getGraphicsQueue()->submit(
                m_buffer,
                m_fence,
                1,
                &m_swapChainReady,
                1,
                &m_renderComplete,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            );

            if (!submitResult) {
                // not totally catastrophic
                return true;
            }
            
            m_frameStarted = false;

            VkSwapchainKHR swap = m_swapChain->get();
            VkPresentInfoKHR pi = {};
            pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            pi.waitSemaphoreCount = 1;
            pi.pWaitSemaphores = &m_renderComplete;
            pi.swapchainCount = 1;
            pi.pSwapchains = &swap;
            pi.pImageIndices = &m_scImageIdx;

            if (vkQueuePresentKHR(m_device->getGraphicsQueue()->get(), &pi) != VK_SUCCESS) {
                // not totally catastrophic
                return true;
            }

            return true;
        }

        bool FrameContext::init(vulkan::SwapChain* swapChain, vulkan::CommandBuffer* cb) {
            m_swapChain = swapChain;
            m_buffer = cb;
            m_device = m_swapChain->getDevice();

            VkSemaphoreCreateInfo si = {};
            si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            if (vkCreateSemaphore(m_device->get(), &si, m_device->getInstance()->getAllocator(), &m_swapChainReady) != VK_SUCCESS) {
                fatal("Failed to create swapchain semaphore.");
                shutdown();
                return false;
            }

            if (vkCreateSemaphore(m_device->get(), &si, m_device->getInstance()->getAllocator(), &m_renderComplete) != VK_SUCCESS) {
                fatal("Failed to create render completion semaphore.");
                shutdown();
                return false;
            }

            VkFenceCreateInfo fi = {};
            fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            if (vkCreateFence(m_device->get(), &fi, m_device->getInstance()->getAllocator(), &m_fence) != VK_SUCCESS) {
                fatal("Failed to create frame fence semaphore.");
                shutdown();
                return false;
            }

            return true;
        }

        void FrameContext::shutdown() {
            if (m_fence) {
                vkDestroyFence(m_device->get(), m_fence, m_device->getInstance()->getAllocator());
                m_fence = VK_NULL_HANDLE;
            }

            if (m_renderComplete) {
                vkDestroySemaphore(m_device->get(), m_renderComplete, m_device->getInstance()->getAllocator());
                m_renderComplete = VK_NULL_HANDLE;
            }

            if (m_swapChainReady) {
                vkDestroySemaphore(m_device->get(), m_swapChainReady, m_device->getInstance()->getAllocator());
                m_swapChainReady = VK_NULL_HANDLE;
            }

            m_swapChain = nullptr;
            m_buffer = nullptr;
            m_framebuffer = nullptr;
            m_swapChainReady = VK_NULL_HANDLE;
            m_renderComplete = VK_NULL_HANDLE;
            m_fence = VK_NULL_HANDLE;
            m_scImageIdx = 0;
            m_frameStarted = false;
        }

        void FrameContext::onAcquire() {
        }

        void FrameContext::onFree() {
            m_scImageIdx = 0;
            m_framebuffer = nullptr;
            m_frameStarted = false;
        }
    };
};