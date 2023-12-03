#include <render/core/FrameContext.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/Pipeline.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/Queue.h>

namespace render {
    namespace core {
        FrameContext::FrameContext() : utils::IWithLogging("Frame") {
            m_pipeline = nullptr;
            m_buffer = nullptr;
            m_swapChainReady = VK_NULL_HANDLE;
            m_renderComplete = VK_NULL_HANDLE;
            m_fence = VK_NULL_HANDLE;
            m_scImageIdx = 0;
            m_frameStarted = false;
        }

        FrameContext::~FrameContext() {
            m_pipeline = nullptr;
            m_buffer = nullptr;
            shutdown();
        }

        vulkan::CommandBuffer* FrameContext::getCommandBuffer() const {
            return m_buffer;
        }

        vulkan::Pipeline* FrameContext::getPipeline() const {
            return m_pipeline;
        }
        
        u32 FrameContext::getSwapChainImageIndex() const {
            return m_scImageIdx;
        }

        bool FrameContext::begin() {
            if (m_frameStarted) return false;

            if (vkWaitForFences(m_device->get(), 1, &m_fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) return false;
            if (vkResetFences(m_device->get(), 1, &m_fence) != VK_SUCCESS) return false;

            if (vkAcquireNextImageKHR(m_device->get(), m_pipeline->getSwapChain()->get(), UINT64_MAX, m_swapChainReady, nullptr, &m_scImageIdx) != VK_SUCCESS) {
                return false;
            }

            if (!m_buffer->reset()) return false;
            if (!m_buffer->begin()) return false;
            m_frameStarted = true;

            return true;
        }

        bool FrameContext::end() {
            if (!m_frameStarted || !m_buffer->end()) return false;

            VkPipelineStageFlags sf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkCommandBuffer buf = m_buffer->get();
            VkSwapchainKHR swap = m_pipeline->getSwapChain()->get();

            VkSubmitInfo si = {};
            si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            si.waitSemaphoreCount = 1;
            si.pWaitSemaphores = &m_swapChainReady;
            si.pWaitDstStageMask = &sf;
            si.commandBufferCount = 1;
            si.pCommandBuffers = &buf;
            si.signalSemaphoreCount = 1;
            si.pSignalSemaphores = &m_renderComplete;

            if (vkQueueSubmit(m_device->getGraphicsQueue()->get(), 1, &si, m_fence) != VK_SUCCESS) {
                // not totally catastrophic
                return true;
            }
            m_frameStarted = false;

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

        bool FrameContext::init(vulkan::LogicalDevice* device) {
            m_device = device;

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
        }

        void FrameContext::onAcquire(vulkan::CommandBuffer* buf, vulkan::Pipeline* pipeline) {
            m_pipeline = pipeline;
            m_buffer = buf;
        }

        void FrameContext::onFree() {
            m_pipeline = nullptr;
            m_buffer = nullptr;
            m_scImageIdx = 0;
            m_frameStarted = false;
        }
    };
};