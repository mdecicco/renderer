#include <render/vulkan/CommandPool.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/QueueFamily.h>
#include <render/vulkan/CommandBuffer.h>

#include <utils/Array.hpp>

namespace render {
    namespace vulkan {
        CommandPool::CommandPool(LogicalDevice* device, const QueueFamily* family) {
            m_device = device;
            m_family = family;
            m_pool = VK_NULL_HANDLE;
        }

        CommandPool::~CommandPool() {
            shutdown();
        }

        bool CommandPool::init(VkCommandPoolCreateFlags flags) {
            if (m_pool) return false;

            VkCommandPoolCreateInfo pi = {};
            pi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            pi.flags = flags;
            pi.queueFamilyIndex = m_family->getIndex();

            if (vkCreateCommandPool(m_device->get(), &pi, m_device->getInstance()->getAllocator(), &m_pool) != VK_SUCCESS) {
                return false;
            }

            m_flags = flags;
            return true;
        }

        void CommandPool::shutdown() {
            if (!m_pool) return;

            m_buffers.each([](CommandBuffer* buf) {
                delete buf;
            });
            m_buffers.clear();

            vkDestroyCommandPool(m_device->get(), m_pool, m_device->getInstance()->getAllocator());
            m_pool = VK_NULL_HANDLE;
        }

        VkCommandPool CommandPool::get() const {
            return m_pool;
        }

        const QueueFamily* CommandPool::getFamily() const {
            return m_family;
        }

        VkCommandPoolCreateFlags CommandPool::getFlags() const {
            return m_flags;
        }

        CommandBuffer* CommandPool::createBuffer(bool primary) {
            VkCommandBufferAllocateInfo cbi = {};
            cbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cbi.commandPool = m_pool;
            cbi.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
            cbi.commandBufferCount = 1;
            
            VkCommandBuffer cb = VK_NULL_HANDLE;
            if (vkAllocateCommandBuffers(m_device->get(), &cbi, &cb) != VK_SUCCESS) return nullptr;

            CommandBuffer* buf = new CommandBuffer();
            buf->m_pool = this;
            buf->m_device = m_device;
            buf->m_buffer = cb;

            m_buffers.push(buf);
            return buf;
        }
    };
};