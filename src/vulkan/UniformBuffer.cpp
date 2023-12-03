#include <render/vulkan/UniformBuffer.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/Pipeline.h>
#include <render/core/DataFormat.h>

#include <utils/Array.hpp>
#include <utils/Math.hpp>

namespace render {
    namespace vulkan {
        //
        // UniformBuffer
        //

        UniformBuffer::UniformBuffer(LogicalDevice* device, core::DataFormat* fmt, u32 objectCapacity) {
            m_device = device;
            m_fmt = fmt;
            m_capacity = objectCapacity;
            m_usedCount = 0;
            m_pool = VK_NULL_HANDLE;
            m_stagingBuffer = VK_NULL_HANDLE;
            m_stagingMemory = VK_NULL_HANDLE;
            m_buffer = VK_NULL_HANDLE;
            m_memory = VK_NULL_HANDLE;
            m_objects = nullptr;
            m_objUpdated = new u8[m_capacity];
            m_hasUpdates = false;
            m_minUpdateIdx = m_capacity;
            m_maxUpdateIdx = 0;

            m_nodes = new UniformObject[m_capacity];
            m_free = m_used = nullptr;

            for (u32 i = 0;i < m_capacity;i++) {
                m_nodes[i].m_buffer = this;
                m_nodes[i].m_index = i;
                m_objUpdated[i] = 0;
            }

            resetNodes();
        }

        UniformBuffer::~UniformBuffer() {
            shutdown();
            
            delete [] m_nodes;
            m_nodes = nullptr;

            delete [] m_objUpdated;
            m_objUpdated = nullptr;
        }

        bool UniformBuffer::init() {
            if (m_buffer) return false;

            VkDescriptorPoolSize ps = {};
            ps.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ps.descriptorCount = m_capacity;

            VkDescriptorPoolCreateInfo pi = {};
            pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pi.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pi.poolSizeCount = 1;
            pi.pPoolSizes = &ps;
            pi.maxSets = m_capacity;

            if (vkCreateDescriptorPool(m_device->get(), &pi, m_device->getInstance()->getAllocator(), &m_pool) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to create descriptor pool for uniform buffer");
                shutdown();
                return false;
            }

            // Staging buffer
            {
                VkBufferCreateInfo bi = {};
                bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bi.size = m_fmt->getSize() * m_capacity;
                bi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                if (vkCreateBuffer(m_device->get(), &bi, m_device->getInstance()->getAllocator(), &m_stagingBuffer) != VK_SUCCESS) {
                    m_device->getInstance()->error("Failed to create staging buffer (%llu bytes)", bi.size);
                    shutdown();
                    return false;
                }

                VkMemoryRequirements memReqs;
                vkGetBufferMemoryRequirements(m_device->get(), m_stagingBuffer, &memReqs);

                auto memProps = m_device->getPhysicalDevice()->getMemoryProperties();
                auto memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                i32 memTypeIdx = -1;
                for (u32 i = 0;i < memProps.memoryTypeCount;i++) {
                    if ((memReqs.memoryTypeBits * (1 << i)) && (memProps.memoryTypes[i].propertyFlags & memFlags) == memFlags) {
                        memTypeIdx = i;
                        break;
                    }
                }

                if (memTypeIdx == -1) {
                    m_device->getInstance()->error("Failed to find correct memory type for staging uniform buffer");
                    shutdown();
                    return false;
                }

                VkMemoryAllocateInfo ai = {};
                ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                ai.allocationSize = memReqs.size;
                ai.memoryTypeIndex = u32(memTypeIdx);

                if (vkAllocateMemory(m_device->get(), &ai, m_device->getInstance()->getAllocator(), &m_stagingMemory) != VK_SUCCESS) {
                    m_device->getInstance()->error("Failed to allocate %llu bytes for staging uniform buffer", ai.allocationSize);
                    shutdown();
                    return false;
                }

                if (vkBindBufferMemory(m_device->get(), m_stagingBuffer, m_stagingMemory, 0) != VK_SUCCESS) {
                    m_device->getInstance()->error("Failed to bind allocated memory to staging uniform buffer");
                    shutdown();
                    return false;
                }
            }

            // GPU buffer
            {
                VkBufferCreateInfo bi = {};
                bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bi.size = m_fmt->getSize() * m_capacity;
                bi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                
                if (vkCreateBuffer(m_device->get(), &bi, m_device->getInstance()->getAllocator(), &m_buffer) != VK_SUCCESS) {
                    m_device->getInstance()->error("Failed to create buffer (%llu bytes)", bi.size);
                    shutdown();
                    return false;
                }

                VkMemoryRequirements memReqs;
                vkGetBufferMemoryRequirements(m_device->get(), m_buffer, &memReqs);

                auto memProps = m_device->getPhysicalDevice()->getMemoryProperties();
                auto memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                i32 memTypeIdx = -1;
                for (u32 i = 0;i < memProps.memoryTypeCount;i++) {
                    if ((memReqs.memoryTypeBits * (1 << i)) && (memProps.memoryTypes[i].propertyFlags & memFlags) == memFlags) {
                        memTypeIdx = i;
                        break;
                    }
                }

                if (memTypeIdx == -1) {
                    m_device->getInstance()->error("Failed to find correct memory type for uniform buffer");
                    shutdown();
                    return false;
                }

                VkMemoryAllocateInfo ai = {};
                ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                ai.allocationSize = memReqs.size;
                ai.memoryTypeIndex = u32(memTypeIdx);

                if (vkAllocateMemory(m_device->get(), &ai, m_device->getInstance()->getAllocator(), &m_memory) != VK_SUCCESS) {
                    m_device->getInstance()->error("Failed to allocate %llu bytes for uniform buffer", ai.allocationSize);
                    shutdown();
                    return false;
                }

                if (vkBindBufferMemory(m_device->get(), m_buffer, m_memory, 0) != VK_SUCCESS) {
                    m_device->getInstance()->error("Failed to bind allocated memory to uniform buffer");
                    shutdown();
                    return false;
                }
            }

            if (vkMapMemory(m_device->get(), m_stagingMemory, 0, VK_WHOLE_SIZE, 0, (void**)&m_objects) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to map staging buffer for writing");
                shutdown();
                return false;
            }

            return true;
        }

        void UniformBuffer::shutdown() {
            if (m_pool) {
                vkDestroyDescriptorPool(m_device->get(), m_pool, m_device->getInstance()->getAllocator());
                m_pool = VK_NULL_HANDLE;
            }

            if (m_objects) {
                vkUnmapMemory(m_device->get(), m_stagingMemory);
                m_objects = nullptr;
            }

            if (m_stagingBuffer) {
                vkDestroyBuffer(m_device->get(), m_stagingBuffer, m_device->getInstance()->getAllocator());
                m_stagingBuffer = VK_NULL_HANDLE;
            }

            if (m_stagingMemory) {
                vkFreeMemory(m_device->get(), m_stagingMemory, m_device->getInstance()->getAllocator());
                m_stagingMemory = VK_NULL_HANDLE;
            }

            if (m_buffer) {
                vkDestroyBuffer(m_device->get(), m_buffer, m_device->getInstance()->getAllocator());
                m_buffer = VK_NULL_HANDLE;
            }

            if (m_memory) {
                vkFreeMemory(m_device->get(), m_memory, m_device->getInstance()->getAllocator());
                m_memory = VK_NULL_HANDLE;
            }

            resetNodes();

            m_hasUpdates = false;
            m_minUpdateIdx = m_capacity;
            m_maxUpdateIdx = 0;
            for (u32 i = 0;i < m_capacity;i++) m_objUpdated[i] = 0;
        }

        LogicalDevice* UniformBuffer::getDevice() const {
            return m_device;
        }

        core::DataFormat* UniformBuffer::getFormat() const {
            return m_fmt;
        }

        VkBuffer UniformBuffer::getBuffer() const {
            return m_buffer;
        }

        VkDeviceMemory UniformBuffer::getMemory() const {
            return m_memory;
        }

        u32 UniformBuffer::getCapacity() const {
            if (!m_memory) return 0;
            return m_capacity;
        }

        u32 UniformBuffer::getRemaining() const {
            return m_capacity - m_usedCount;
        }

        UniformObject* UniformBuffer::allocate(u32 bindIndex, Pipeline* pipeline) {
            if (!m_memory || !m_free) return nullptr;
            UniformObject* n = m_free;

            VkDescriptorSetLayout layout = pipeline->getDescriptorSetLayout();

            VkDescriptorSetAllocateInfo ai = {};
            ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            ai.descriptorPool = m_pool;
            ai.descriptorSetCount = 1;
            ai.pSetLayouts = &layout;

            if (vkAllocateDescriptorSets(m_device->get(), &ai, &n->m_set) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to allocate descriptor set for uniform object");
                return nullptr;
            }

            n->m_bindIdx = bindIndex;

            VkDescriptorBufferInfo bi = {};
            bi.buffer = m_buffer;
            bi.offset = n->m_index * m_fmt->getSize();
            bi.range = m_fmt->getSize();

            VkWriteDescriptorSet wd = {};
            wd.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            wd.dstSet = n->m_set;
            wd.dstBinding = bindIndex;
            wd.dstArrayElement = 0;
            wd.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            wd.descriptorCount = 1;
            wd.pBufferInfo = &bi;

            vkUpdateDescriptorSets(m_device->get(), 1, &wd, 0, nullptr);

            m_free = n->m_next;
            if (m_free) m_free->m_last = nullptr;
            
            n->m_next = m_used;
            if (m_used) m_used->m_last = n;
            m_used = n;

            m_usedCount++;
            return n;
        }

        void UniformBuffer::free(UniformObject* n) {
            if (!m_memory || !m_used || !n || n->m_buffer != this) return;

            if (vkFreeDescriptorSets(m_device->get(), m_pool, 1, &n->m_set) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to free descriptor set for uniform object");
            }

            n->m_set = VK_NULL_HANDLE;
            n->m_bindIdx = 0;

            if (n->m_last) n->m_last->m_next = n->m_next;
            if (n->m_next) n->m_next->m_last = n->m_last;

            if (n == m_used) m_used = n->m_next;
            
            insertToFreeList(n);
            m_usedCount--;
        }

        void UniformBuffer::submitUpdates(CommandBuffer* cb) {
            if (!m_memory || !m_hasUpdates) return;

            u32 objSize = m_fmt->getSize();

            m_copyRanges.clear(false);
            VkBufferCopy* cur = nullptr;
            bool doStartNewRange = true;

            for (u32 i = m_minUpdateIdx;i <= m_maxUpdateIdx;i++) {
                if (!m_objUpdated[i]) {
                    doStartNewRange = true;
                    continue;
                }

                m_objUpdated[i] = false;

                if (doStartNewRange) {
                    m_copyRanges.emplace();
                    cur = &m_copyRanges.last();
                    cur->srcOffset = cur->dstOffset = i * objSize;
                    cur->size = objSize;
                    doStartNewRange = false;
                    continue;
                }

                cur->size += objSize;
            }

            vkCmdCopyBuffer(cb->get(), m_stagingBuffer, m_buffer, m_copyRanges.size(), m_copyRanges.data());
            m_hasUpdates = false;
            m_minUpdateIdx = m_capacity;
            m_maxUpdateIdx = 0;
        }

        void UniformBuffer::resetNodes() {
            for (u32 i = 0;i < m_capacity;i++) {
                if (i > 0) {
                    m_nodes[i].m_last = &m_nodes[i - 1];
                    m_nodes[i].m_next = nullptr;
                    m_nodes[i - 1].m_next = &m_nodes[i];
                } else {
                    m_nodes[i].m_last = nullptr;
                    m_nodes[i].m_next = nullptr;
                }
            }

            UniformObject* initial = &m_nodes[0];

            m_free = initial;
            m_used = nullptr;

            initial->m_last = nullptr;
            initial->m_next = nullptr;
            m_usedCount = 0;
        }

        void UniformBuffer::insertToFreeList(UniformObject* n) {
            UniformObject* i = m_free;
            while (i) {
                if (i->m_index > n->m_index) {
                    // this node, and all that follow are after node n. n should be first
                    i = nullptr;
                    break;
                }

                if (!i->m_next || i->m_next->m_index >= n->m_index) {
                    // the next node comes after n, or n should be the last node
                    break;
                }

                i = i->m_next;
            }

            if (i) {
                // insert n after i
                n->m_next = i->m_next;
                n->m_last = i;
                i->m_next = n;
                if (n->m_next) n->m_next->m_last = n;
            } else {
                // insert n at start
                n->m_next = m_free;
                n->m_last = nullptr;
                m_free = n;
            }
        }
        
        void UniformBuffer::updateObject(UniformObject* n, const void* data) {
            u32 idx = n->m_index;
            u32 sz = m_fmt->getSize();
            memcpy(m_objects + (idx * sz), data, sz);
            m_hasUpdates = true;
            m_objUpdated[idx] = 1;
            m_minUpdateIdx = utils::min(m_minUpdateIdx, idx);
            m_maxUpdateIdx = utils::max(m_maxUpdateIdx, idx);
        }



        //
        // UniformObject
        //

        UniformObject::UniformObject() {
            m_set = VK_NULL_HANDLE;
            m_buffer = nullptr;
            m_index = 0;
            m_bindIdx = 0;
        }

        UniformObject::~UniformObject() {
        }

        UniformBuffer* UniformObject::getBuffer() const {
            return m_buffer;
        }

        u32 UniformObject::getBindIndex() const {
            return m_bindIdx;
        }

        VkDescriptorSet UniformObject::getDescriptorSet() const {
            return m_set;
        }

        void UniformObject::free() {
            m_buffer->free(this);
        }



        //
        // UniformBufferFactory
        //

        UniformBufferFactory::UniformBufferFactory(LogicalDevice* device, u32 maxBufferObjectCapacity) {
            m_device = device;
            m_maxBufObjCapacity = maxBufferObjectCapacity;
        }

        UniformBufferFactory::~UniformBufferFactory() {
            freeAll();
        }

        void UniformBufferFactory::freeAll() {
            m_formats.clear();
            m_buffers.each([](utils::Array<UniformBuffer*>& arr) {
                arr.each([](UniformBuffer* buf) {
                    delete buf;
                });
            });
            m_buffers.clear();
        }

        UniformObject* UniformBufferFactory::allocate(core::DataFormat* fmt, u32 bindIndex, Pipeline* pipeline) {
            i32 idx = m_formats.findIndex([fmt](core::DataFormat* f) {
                return (*f) == (*fmt);
            });

            if (idx == -1) {
                UniformBuffer* buf = new UniformBuffer(m_device, fmt, m_maxBufObjCapacity);
                if (!buf->init()) {
                    delete buf;
                    return nullptr;
                }

                m_formats.push(fmt);
                m_buffers.push({ buf });

                return buf->allocate(bindIndex, pipeline);
            }

            utils::Array<UniformBuffer*>& arr = m_buffers[u32(idx)];
            UniformBuffer* buf = arr.find([](UniformBuffer* b) {
                return b->getRemaining() > 0;
            });

            if (buf) return buf->allocate(bindIndex, pipeline);
            
            buf = new UniformBuffer(m_device, fmt, m_maxBufObjCapacity);
            if (!buf->init()) {
                delete buf;
                return nullptr;
            }

            arr.push(buf);
            return buf->allocate(bindIndex, pipeline);
        }
    };
};