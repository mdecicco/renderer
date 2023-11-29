#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/CommandPool.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/QueueFamily.h>
#include <render/vulkan/Pipeline.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/VertexBuffer.h>

#include <utils/Array.hpp>

namespace render {
    namespace vulkan {
        CommandBuffer::CommandBuffer() {
            m_pool = nullptr;
            m_buffer = VK_NULL_HANDLE;
            m_isRecording = false;
        }

        CommandBuffer::~CommandBuffer() {
        }

        VkCommandBuffer CommandBuffer::get() const {
            return m_buffer;
        }

        CommandPool* CommandBuffer::getPool() const {
            return m_pool;
        }

        bool CommandBuffer::begin(VkCommandBufferUsageFlagBits flags) {
            if (!m_buffer || m_isRecording) return false;
            VkCommandBufferBeginInfo bi = {};
            bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            bi.flags = flags;

            // todo
            bi.pInheritanceInfo = VK_NULL_HANDLE;

            VkResult result = vkBeginCommandBuffer(m_buffer, &bi);
            if (result == VK_SUCCESS) {
                m_isRecording = true;
                return true;
            }

            return false;
        }

        bool CommandBuffer::end() {
            if (!m_buffer || !m_isRecording) return false;

            VkResult result = vkEndCommandBuffer(m_buffer);
            if (result == VK_SUCCESS) {
                m_isRecording = false;
                return true;
            }

            return false;
        }
        
        bool CommandBuffer::reset() {
            if (!m_buffer) return false;
            if ((m_pool->getFlags() & VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) == 0) {
                // The whole pool needs to be reset
                return false;
            }

            return vkResetCommandBuffer(m_buffer, 0) == VK_SUCCESS;
        }

        void CommandBuffer::beginRenderPass(Pipeline* pipeline, const VkClearValue& clearColor, u32 imageIdx) {
            if (!m_buffer || !m_isRecording) return;

            VkRenderPassBeginInfo rpi = {};
            rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rpi.renderPass = pipeline->getRenderPass();
            rpi.framebuffer = pipeline->getFramebuffers()[imageIdx];
            rpi.renderArea.offset.x = 0;
            rpi.renderArea.offset.y = 0;
            rpi.renderArea.extent = pipeline->getSwapChain()->getExtent();
            rpi.clearValueCount = 1;
            rpi.pClearValues = &clearColor;

            vkCmdBeginRenderPass(
                m_buffer,
                &rpi,
                // todo
                VK_SUBPASS_CONTENTS_INLINE
            );
        }

        void CommandBuffer::endRenderPass() {
            if (!m_buffer || !m_isRecording) return;

            vkCmdEndRenderPass(m_buffer);
        }
        
        void CommandBuffer::bindPipeline(Pipeline* pipeline, VkPipelineBindPoint bindPoint) {
            if (!m_buffer || !m_isRecording) return;

            vkCmdBindPipeline(m_buffer, bindPoint, pipeline->get());
        }
        
        void CommandBuffer::bindVertexBuffer(VertexBuffer* vbo) {
            if (!m_buffer || !m_isRecording) return;
            VkDeviceSize offset = 0;
            VkBuffer buf = vbo->getBuffer();
            vkCmdBindVertexBuffers(m_buffer, 0, 1, &buf, &offset);
        }

        void CommandBuffer::setViewport(f32 x, f32 y, f32 width, f32 height, f32 minZ, f32 maxZ) {
            if (!m_buffer || !m_isRecording) return;

            VkViewport vp = {};
            vp.x = x;
            vp.y = y;
            vp.width = width;
            vp.height = height;
            vp.minDepth = minZ;
            vp.maxDepth = maxZ;

            vkCmdSetViewport(m_buffer, 0, 1, &vp);
        }

        void CommandBuffer::setScissor(i32 x, i32 y, u32 width, u32 height) {
            if (!m_buffer || !m_isRecording) return;
            VkRect2D s = {};
            s.offset.x = x;
            s.offset.y = y;
            s.extent.width = width;
            s.extent.height = height;

            vkCmdSetScissor(m_buffer, 0, 1, &s);
        }
        
        void CommandBuffer::draw(u32 vertexCount, u32 firstVertex, u32 instanceCount, u32 firstInstance) {
            if (!m_buffer || !m_isRecording) return;
            vkCmdDraw(m_buffer, vertexCount, instanceCount, firstVertex, firstInstance);
        }

        void CommandBuffer::draw(Vertices* vertices) {
            if (!m_buffer || !m_isRecording) return;
            vkCmdDraw(m_buffer, vertices->getCount(), 1, vertices->getOffset(), 0);
        }
    };
};