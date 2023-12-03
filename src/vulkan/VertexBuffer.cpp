#include <render/vulkan/VertexBuffer.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/core/DataFormat.h>

#include <utils/Array.hpp>

#define MAX_NODE_COUNT 1024
#define MIN_NODE_SIZE 3

namespace render {
    namespace vulkan {
        //
        // VertexBuffer
        //

        VertexBuffer::VertexBuffer(LogicalDevice* device, core::DataFormat* fmt, u32 vertexCapacity) {
            m_device = device;
            m_fmt = fmt;
            m_capacity = vertexCapacity;
            m_currentMaximumBlockSize = vertexCapacity;
            m_memoryMapRefCount = 0;
            m_nodeCount = MAX_NODE_COUNT;
            m_buffer = VK_NULL_HANDLE;
            m_memory = VK_NULL_HANDLE;

            m_unusedNodes = m_nodes = new node[m_nodeCount];
            m_freeBlocks = m_usedBlocks = nullptr;

            for (u32 i = 0;i < m_nodeCount;i++) m_nodes[i].verts = nullptr;

            resetNodes();
        }

        VertexBuffer::~VertexBuffer() {
            shutdown();
            
            delete [] m_nodes;
            m_nodes = nullptr;
        }

        bool VertexBuffer::init() {
            if (m_buffer) return false;

            VkBufferCreateInfo bi = {};
            bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bi.size = m_fmt->getSize() * m_capacity;
            bi.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(m_device->get(), &bi, m_device->getInstance()->getAllocator(), &m_buffer) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to create buffer (%llu bytes)", bi.size);
                return false;
            }

            VkMemoryRequirements memReqs;
            vkGetBufferMemoryRequirements(m_device->get(), m_buffer, &memReqs);

            VkMemoryPropertyFlags memPropFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

            auto memProps = m_device->getPhysicalDevice()->getMemoryProperties();
            i32 memTypeIdx = -1;
            for (u32 i = 0;i < memProps.memoryTypeCount;i++) {
                if ((memReqs.memoryTypeBits * (1 << i)) && (memProps.memoryTypes[i].propertyFlags & memPropFlags) == memPropFlags) {
                    memTypeIdx = i;
                    break;
                }
            }

            if (memTypeIdx == -1) {
                m_device->getInstance()->error("Failed to find correct memory type for vertex buffer");
                shutdown();
                return false;
            }

            VkMemoryAllocateInfo ai = {};
            ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize = memReqs.size;
            ai.memoryTypeIndex = u32(memTypeIdx);

            if (vkAllocateMemory(m_device->get(), &ai, m_device->getInstance()->getAllocator(), &m_memory) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to allocate %llu bytes for vertex buffer", ai.allocationSize);
                shutdown();
                return false;
            }

            if (vkBindBufferMemory(m_device->get(), m_buffer, m_memory, 0) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to bind allocated memory to vertex buffer");
                shutdown();
                return false;
            }

            return true;
        }

        void VertexBuffer::shutdown() {
            if (m_buffer) {
                vkDestroyBuffer(m_device->get(), m_buffer, m_device->getInstance()->getAllocator());
                m_buffer = VK_NULL_HANDLE;
            }

            if (m_memory) {
                vkFreeMemory(m_device->get(), m_memory, m_device->getInstance()->getAllocator());
                m_memory = nullptr;
            }

            resetNodes();
        }

        LogicalDevice* VertexBuffer::getDevice() const {
            return m_device;
        }

        core::DataFormat* VertexBuffer::getFormat() const {
            return m_fmt;
        }

        VkBuffer VertexBuffer::getBuffer() const {
            return m_buffer;
        }

        VkDeviceMemory VertexBuffer::getMemory() const {
            return m_memory;
        }

        u32 VertexBuffer::getCapacity() const {
            if (!m_memory) return 0;
            return m_capacity;
        }

        u32 VertexBuffer::getCurrentMaximumBlockSize() const {
            if (!m_memory) return 0;
            return m_currentMaximumBlockSize;
        }

        Vertices* VertexBuffer::allocate(u32 count) {
            if (!m_memory || !m_freeBlocks) return nullptr;

            node* n = getNode(count);
            if (!n) return nullptr;

            u32 originalBlockSize = n->size;

            // spawn new node if there's a reasonable amount of remaining space (and more nodes left...)
            if (n->size > count && (n->size - count) > MIN_NODE_SIZE && m_unusedNodes) {
                node* split = m_unusedNodes;
                if (split->next) split->next->last = nullptr;
                m_unusedNodes = split->next;
                split->next = nullptr;

                split->offset = n->offset + count;
                split->size = n->size - count;
                n->size -= split->size;

                insertToFreeList(split);
            }

            if (originalBlockSize >= m_currentMaximumBlockSize) {
                recalculateMaximumBlockSize();
            }

            n->verts = new Vertices(this, m_fmt, n);
            return n->verts;
        }

        void VertexBuffer::free(Vertices* verts) {
            if (!m_memory || !m_usedBlocks || !verts || verts->m_buffer != this) return;

            node* n = verts->m_node;

            delete n->verts;
            n->verts = nullptr;

            if (n->last) n->last->next = n->next;
            if (n->next) n->next->last = n->last;

            if (n == m_usedBlocks) m_usedBlocks = n->next;
            
            insertToFreeList(n);
        }

        void VertexBuffer::recalculateMaximumBlockSize() {
            if (!m_memory || !m_usedBlocks) return;

            m_currentMaximumBlockSize = 0;

            node* n = m_freeBlocks;
            while (n) {
                if (n->size > m_currentMaximumBlockSize) m_currentMaximumBlockSize = n->size;
                n = n->next;
            }
        }

        void VertexBuffer::resetNodes() {
            for (u32 i = 0;i < m_nodeCount;i++) {
                if (m_nodes[i].verts) delete m_nodes[i].verts;
                m_nodes[i].verts = nullptr;
                m_nodes[i].offset = 0;
                m_nodes[i].size = 0;

                if (i > 0) {
                    m_nodes[i].last = &m_nodes[i - 1];
                    m_nodes[i].next = nullptr;
                    m_nodes[i - 1].next = &m_nodes[i];
                } else {
                    m_nodes[i].last = nullptr;
                    m_nodes[i].next = nullptr;
                }
            }

            node* initial = &m_nodes[0];

            m_unusedNodes = initial->next;
            m_freeBlocks = initial;
            m_usedBlocks = nullptr;
            if (m_unusedNodes) m_unusedNodes->last = nullptr;

            initial->last = nullptr;
            initial->next = nullptr;
            initial->offset = 0;
            initial->size = m_capacity;

            m_currentMaximumBlockSize = m_capacity;
        }
    
        VertexBuffer::node* VertexBuffer::getNode(u32 vcount) {
            node* minNode = nullptr;
            node* n = m_freeBlocks;

            while (n) {
                if (n->size >= vcount && (!minNode || n->size < minNode->size)) minNode = n;
                n = n->next;
            }

            if (!minNode) return nullptr;

            if (minNode == m_freeBlocks) {
                m_freeBlocks = minNode->next;
                if (m_freeBlocks) m_freeBlocks->last = nullptr;
            }

            if (minNode->last) minNode->last->next = minNode->next;
            if (minNode->next) minNode->next->last = minNode->last;
            
            minNode->next = m_usedBlocks;
            if (m_usedBlocks) m_usedBlocks->last = minNode;
            m_usedBlocks = minNode;

            return minNode;
        }

        void VertexBuffer::recycleNode(node* n) {
            n->verts = nullptr;
            n->offset = n->size = 0;

            if (n->last) n->last->next = n->next;
            if (n->next) n->next->last = n->last;

            n->last = nullptr;
            n->next = m_unusedNodes;
            if (n->next) n->next->last = n;

            m_unusedNodes = n;
        }
    
        void VertexBuffer::insertToFreeList(node* n) {
            node* i = m_freeBlocks;
            while (i) {
                if (i->offset + i->size > n->offset) {
                    // this node, and all that follow are after node n. n should be first
                    i = nullptr;
                    break;
                }

                if (!i->next || i->next->offset >= n->offset + n->size) {
                    // the next node comes after n, or n should be the last node
                    break;
                }

                i = i->next;
            }

            if (i) {
                // insert n after i
                n->next = i->next;
                n->last = i;
                i->next = n;
                if (n->next) n->next->last = n;
            } else {
                // insert n at start
                n->next = m_freeBlocks;
                n->last = nullptr;
                m_freeBlocks = n;
            }

            // join with last node if adjacent
            if (n->last && n->last->offset + n->last->size == n->offset) {
                // previous node ends at start of current node
                n->last->size += n->size;

                node* tmp = n->last;
                recycleNode(n);
                n = tmp;
            }

            // join with next node if adjacent
            if (n->next && n->offset + n->size == n->next->offset) {
                // next node starts at the end of current node
                n->size += n->next->size;
                recycleNode(n->next);
            }

            if (n->size > m_currentMaximumBlockSize) {
                m_currentMaximumBlockSize = n->size;
            }
        }
    

        //
        // Vertices
        //

        Vertices::Vertices(VertexBuffer* buf, core::DataFormat* fmt, VertexBuffer::node* n) {
            m_buffer = buf;
            m_fmt = fmt;
            m_node = n;
            m_mappedMemory = nullptr;

            u32 nonCoherentAtomSize = buf->m_device->getPhysicalDevice()->getProperties().limits.nonCoherentAtomSize;

            m_range = {};
            m_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            m_range.memory = m_buffer->m_memory;
            m_range.offset = m_node->offset * m_fmt->getSize();
            m_range.size = m_node->size * m_fmt->getSize();

            if (nonCoherentAtomSize > 0) {
                m_range.size += (nonCoherentAtomSize - (m_range.size % nonCoherentAtomSize));
            }
        }

        Vertices::~Vertices() {
        }

        u32 Vertices::getOffset() const {
            return m_node->offset;
        }

        u32 Vertices::getByteOffset() const {
            return m_node->offset * m_fmt->getSize();
        }

        u32 Vertices::getSize() const {
            return m_node->size * m_fmt->getSize();
        }

        u32 Vertices::getCount() const {
            return m_node->size;
        }

        VertexBuffer* Vertices::getBuffer() const {
            return m_buffer;
        }

        void Vertices::free() {
            m_buffer->free(this);
        }

        bool Vertices::beginUpdate(bool willReadData) {
            if (m_mappedMemory) return false;

            VkResult r = vkMapMemory(
                m_buffer->m_device->get(),
                m_buffer->m_memory,
                m_range.offset,
                m_range.size,
                0,
                &m_mappedMemory
            );

            if (r != VK_SUCCESS) return false;

            if (willReadData) {
                if (vkInvalidateMappedMemoryRanges(m_buffer->m_device->get(), 1, &m_range) != VK_SUCCESS) {
                    m_buffer->m_device->getInstance()->warn(
                        "Vertices::beginUpdate successfully mapped memory, but failed to set up mapped memory for reading"
                    );
                }
            }

            
            m_buffer->m_memoryMapRefCount++;

            return true;
        }
        
        void Vertices::write(void* data, u32 offset, u32 count) {
            if (!m_mappedMemory) return;

            u32 bufSz = m_node->size * m_fmt->getSize();
            u32 begin = offset * m_fmt->getSize();
            u32 end = begin + (count * m_fmt->getSize());

            if (begin >= bufSz) return;
            if (end > bufSz) end = bufSz;

            memcpy((u8*)m_mappedMemory + begin, data, end - begin);
        }

        bool Vertices::commitUpdate() {
            if (!m_mappedMemory) return false;

            if (m_buffer->m_memoryMapRefCount == 0) {
                m_buffer->m_device->getInstance()->warn("Vertices::commitUpdate called more times than Vertices::beginUpdate");
                return false;
            }

            m_buffer->m_memoryMapRefCount--;

            bool result = vkFlushMappedMemoryRanges(m_buffer->m_device->get(), 1, &m_range) == VK_SUCCESS;

            if (m_buffer->m_memoryMapRefCount == 0) {
                vkUnmapMemory(m_buffer->m_device->get(), m_buffer->m_memory);
            }

            m_mappedMemory = nullptr;

            return result;
        }


        //
        // VertexBufferFactory
        //

        VertexBufferFactory::VertexBufferFactory(LogicalDevice* device, u32 maxBufferCapacity) {
            m_device = device;
            m_maxBufCapacity = maxBufferCapacity;
        }

        VertexBufferFactory::~VertexBufferFactory() {
            freeAll();
        }

        void VertexBufferFactory::freeAll() {
            m_formats.clear();
            m_buffers.each([](utils::Array<VertexBuffer*>& arr) {
                arr.each([](VertexBuffer* buf) {
                    delete buf;
                });
            });
            m_buffers.clear();
        }

        Vertices* VertexBufferFactory::allocate(core::DataFormat* fmt, u32 count) {
            if (count >= m_maxBufCapacity) {
                m_device->getInstance()->error("Attempted to allocate more vertices than allowed by VertexBufferFactory (%lu)", m_maxBufCapacity);
                return nullptr;
            }

            i32 idx = m_formats.findIndex([fmt](core::DataFormat* f) {
                return (*f) == (*fmt);
            });

            if (idx == -1) {
                VertexBuffer* buf = new VertexBuffer(m_device, fmt, m_maxBufCapacity);
                if (!buf->init()) {
                    delete buf;
                    return nullptr;
                }

                m_formats.push(fmt);
                m_buffers.push({ buf });

                return buf->allocate(count);
            }

            utils::Array<VertexBuffer*>& arr = m_buffers[u32(idx)];
            VertexBuffer* buf = arr.find([count](VertexBuffer* b) {
                return b->getCurrentMaximumBlockSize() >= count;
            });

            if (buf) return buf->allocate(count);
            
            buf = new VertexBuffer(m_device, fmt, m_maxBufCapacity);
            if (!buf->init()) {
                delete buf;
                return nullptr;
            }

            arr.push(buf);
            return buf->allocate(count);
        }
    };
};