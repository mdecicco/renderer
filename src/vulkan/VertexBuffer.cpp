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

        VertexBuffer::VertexBuffer(LogicalDevice* device, core::DataFormat* fmt, u32 vertexCapacity) : m_buffer(device) {
            m_device = device;
            m_fmt = fmt;
            m_capacity = vertexCapacity;
            m_currentMaximumBlockSize = vertexCapacity;
            m_memoryMapRefCount = 0;
            m_nodeCount = MAX_NODE_COUNT;

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
            if (m_buffer.isValid()) return false;

            return m_buffer.init(
                m_fmt->getSize() * m_capacity,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_SHARING_MODE_EXCLUSIVE,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );
        }

        void VertexBuffer::shutdown() {
            m_buffer.shutdown();
            resetNodes();
        }

        LogicalDevice* VertexBuffer::getDevice() const {
            return m_device;
        }

        core::DataFormat* VertexBuffer::getFormat() const {
            return m_fmt;
        }

        VkBuffer VertexBuffer::getBuffer() const {
            return m_buffer.get();
        }

        VkDeviceMemory VertexBuffer::getMemory() const {
            return m_buffer.getMemory();
        }

        u32 VertexBuffer::getCapacity() const {
            if (!m_buffer.isValid()) return 0;
            return m_capacity;
        }

        u32 VertexBuffer::getCurrentMaximumBlockSize() const {
            if (!m_buffer.isValid()) return 0;
            return m_currentMaximumBlockSize;
        }

        Vertices* VertexBuffer::allocate(u32 count) {
            if (!m_buffer.isValid() || !m_freeBlocks) return nullptr;

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
            if (!m_buffer.isValid() || !m_usedBlocks || !verts || verts->m_buffer != this) return;

            node* n = verts->m_node;

            delete n->verts;
            n->verts = nullptr;

            if (n->last) n->last->next = n->next;
            if (n->next) n->next->last = n->last;

            if (n == m_usedBlocks) m_usedBlocks = n->next;
            
            insertToFreeList(n);
        }

        void VertexBuffer::recalculateMaximumBlockSize() {
            if (!m_buffer.isValid() || !m_usedBlocks) return;

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

        bool Vertices::beginUpdate() {
            if (m_buffer->m_memoryMapRefCount == 0) {
                if (!m_buffer->m_buffer.map()) return false;
            }
            
            m_buffer->m_memoryMapRefCount++;
            return true;
        }
        
        bool Vertices::write(void* data, u32 offset, u32 count) {
            return m_buffer->m_buffer.write(
                data,
                offset * m_fmt->getSize(),
                count * m_fmt->getSize()
            );
        }

        bool Vertices::commitUpdate() {
            if (m_buffer->m_memoryMapRefCount == 0) {
                m_buffer->m_device->getInstance()->warn("Vertices::commitUpdate called more times than Vertices::beginUpdate");
                return false;
            }

            bool r = m_buffer->m_buffer.flush(
                m_node->offset * m_fmt->getSize(),
                m_node->size * m_fmt->getSize()
            );
            
            m_buffer->m_memoryMapRefCount--;
            if (m_buffer->m_memoryMapRefCount == 0) m_buffer->m_buffer.unmap();

            return r;
        }


        //
        // VertexBufferFactory
        //

        VertexBufferFactory::VertexBufferFactory(LogicalDevice* device, u32 minBufferCapacity) {
            m_device = device;
            m_minBufferCapacity = minBufferCapacity;
        }

        VertexBufferFactory::~VertexBufferFactory() {
            freeAll();
        }

        void VertexBufferFactory::freeAll() {
            m_formats.clear();
            m_buffers.each([](Array<VertexBuffer*>& arr) {
                arr.each([](VertexBuffer* buf) {
                    delete buf;
                });
            });
            m_buffers.clear();
        }

        Vertices* VertexBufferFactory::allocate(core::DataFormat* fmt, u32 count) {
            i32 idx = m_formats.findIndex([fmt](core::DataFormat* f) {
                return f->isEqualTo(fmt);
            });

            if (idx == -1) {
                VertexBuffer* buf = new VertexBuffer(m_device, fmt, utils::max(m_minBufferCapacity, count));
                if (!buf->init()) {
                    delete buf;
                    return nullptr;
                }

                m_formats.push(fmt);
                m_buffers.push({ buf });

                return buf->allocate(count);
            }

            Array<VertexBuffer*>& arr = m_buffers[u32(idx)];
            VertexBuffer* buf = arr.find([count](VertexBuffer* b) {
                return b->getCurrentMaximumBlockSize() >= count;
            });

            if (buf) return buf->allocate(count);
            
            buf = new VertexBuffer(m_device, fmt, utils::max(m_minBufferCapacity, count));
            if (!buf->init()) {
                delete buf;
                return nullptr;
            }

            arr.push(buf);
            return buf->allocate(count);
        }
    };
};