#include <render/vulkan/Buffer.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/Instance.h>

namespace render {
    namespace vulkan {
        Buffer::Buffer(LogicalDevice* device) {
            m_device = device;
            m_size = 0;
            m_buffer = VK_NULL_HANDLE;
            m_memory = VK_NULL_HANDLE;
            m_usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
            m_sharingMode = VK_SHARING_MODE_MAX_ENUM;
            m_memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            m_mappedMemory = nullptr;
        }

        Buffer::~Buffer() {
            shutdown();
        }
        
        LogicalDevice* Buffer::getDevice() const {
            return m_device;
        }
        
        VkBuffer Buffer::get() const {
            return m_buffer;
        }
        
        VkDeviceMemory Buffer::getMemory() const {
            return m_memory;
        }
        
        VkBufferUsageFlags Buffer::getUsage() const {
            return m_usage;
        }
        
        VkSharingMode Buffer::getSharingMode() const {
            return m_sharingMode;
        }
        
        VkMemoryPropertyFlags Buffer::getMemoryFlags() const {
            return m_memoryFlags;
        }
        
        bool Buffer::getRange(u64 offset, u64 size, VkMappedMemoryRange& range) const {
            range = {};
            if (offset + size >= m_size) return false;

            if (size == VK_WHOLE_SIZE) offset = 0;
            else {
                u32 nonCoherentAtomSize = m_device->getPhysicalDevice()->getProperties().limits.nonCoherentAtomSize;
                if (nonCoherentAtomSize > 0) {
                    size += (nonCoherentAtomSize - (size % nonCoherentAtomSize));
                }
            }

            if (offset + size >= m_size) return false;

            range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            range.pNext = VK_NULL_HANDLE;
            range.memory = m_memory;
            range.offset = offset;
            range.size = size;

            return true;
        }

        u64 Buffer::getSize() const {
            return m_size;
        }
        
        bool Buffer::isValid() const {
            return m_buffer != VK_NULL_HANDLE && m_memory != VK_NULL_HANDLE;
        }
        
        bool Buffer::map() {
            if (m_mappedMemory) return false;
            return vkMapMemory(m_device->get(), m_memory, 0, VK_WHOLE_SIZE, 0, &m_mappedMemory) == VK_SUCCESS;
        }
        
        bool Buffer::flush(u64 offset, u64 size) {
            if (!m_mappedMemory) return false;
            if (offset + size >= m_size) return false;

            VkMappedMemoryRange range;
            if (!getRange(offset, size, range)) return false;

            return vkFlushMappedMemoryRanges(m_device->get(), 1, &range) == VK_SUCCESS;
        }
        
        bool Buffer::write(const void* src, u64 offset, u64 size) {
            if (!m_mappedMemory) return false;
            if (offset + size >= m_size) return false;

            memcpy(((u8*)m_mappedMemory) + offset, src, size);
            return true;
        }
        
        bool Buffer::read(u64 offset, u64 size, void* dst, bool fetchFromDevice) {
            if (!m_mappedMemory) return false;
            if (offset + size >= m_size) return false;

            if (fetchFromDevice) {
                VkMappedMemoryRange range;
                if (!getRange(offset, size, range)) return false;
                if (vkInvalidateMappedMemoryRanges(m_device->get(), 1, &range) != VK_SUCCESS) return false;
            }

            memcpy(dst, ((u8*)m_mappedMemory) + offset, size);
            return true;
        }
        
        void* Buffer::getPointer(u64 offset) {
            if (!m_mappedMemory) return nullptr;
            if (offset >= m_size) return nullptr;

            return ((u8*)m_mappedMemory) + offset;
        }

        void Buffer::unmap() {
            if (!m_mappedMemory) return;
            vkUnmapMemory(m_device->get(), m_memory);
            m_mappedMemory = nullptr;
        }

        bool Buffer::init(u64 size, VkBufferUsageFlags usage, VkSharingMode sharingMode, VkMemoryPropertyFlags memoryFlags) {
            m_size = size;
            m_usage = usage;
            m_sharingMode = sharingMode;
            m_memoryFlags = memoryFlags;

            VkBufferCreateInfo bi = {};
            bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bi.size = m_size;
            bi.usage = m_usage;
            bi.sharingMode = m_sharingMode;

            if (vkCreateBuffer(m_device->get(), &bi, m_device->getInstance()->getAllocator(), &m_buffer) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to create buffer (%llu bytes)", bi.size);
                shutdown();
                return false;
            }

            VkMemoryRequirements memReqs;
            vkGetBufferMemoryRequirements(m_device->get(), m_buffer, &memReqs);

            VkMemoryAllocateInfo ai = {};
            ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize = memReqs.size;

            if (!m_device->getPhysicalDevice()->getMemoryTypeIndex(memReqs, m_memoryFlags, &ai.memoryTypeIndex)) {
                m_device->getInstance()->error("Failed to find correct memory type for buffer");
                shutdown();
                return false;
            }

            if (vkAllocateMemory(m_device->get(), &ai, m_device->getInstance()->getAllocator(), &m_memory) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to allocate %llu bytes for buffer", ai.allocationSize);
                shutdown();
                return false;
            }

            if (vkBindBufferMemory(m_device->get(), m_buffer, m_memory, 0) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to bind allocated memory to buffer");
                shutdown();
                return false;
            }

            return true;
        }
        
        void Buffer::shutdown() {
            if (m_buffer) {
                vkDestroyBuffer(m_device->get(), m_buffer, m_device->getInstance()->getAllocator());
                m_buffer = VK_NULL_HANDLE;
            }

            if (m_memory) {
                vkFreeMemory(m_device->get(), m_memory, m_device->getInstance()->getAllocator());
                m_memory = VK_NULL_HANDLE;
            }
            
            m_size = 0;
            m_usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
            m_sharingMode = VK_SHARING_MODE_MAX_ENUM;
            m_memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            m_mappedMemory = nullptr;
        }
    };
};