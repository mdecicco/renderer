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

        UniformBuffer::UniformBuffer(LogicalDevice* device, core::DataFormat* fmt, u32 objectCapacity) : m_buffer(device), m_stagingBuffer(device) {
            m_device = device;
            m_fmt = fmt;
            m_capacity = objectCapacity;
            m_usedCount = 0;
            m_objects = nullptr;
            m_objUpdated = new u8[m_capacity];
            m_hasUpdates = false;
            m_minUpdateIdx = m_capacity;
            m_maxUpdateIdx = 0;
            m_paddedObjectSize = m_fmt->getUniformBlockSize();

            u32 alignment = device->getPhysicalDevice()->getProperties().limits.minUniformBufferOffsetAlignment;
            if (alignment > 0) m_paddedObjectSize = (m_paddedObjectSize + alignment - 1) & ~(alignment - 1);

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
            if (m_buffer.isValid()) return false;

            bool r;
            
            r = m_stagingBuffer.init(
                m_paddedObjectSize * m_capacity,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_SHARING_MODE_EXCLUSIVE,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            if (!r) {
                shutdown();
                return false;
            }
            
            r = m_buffer.init(
                m_paddedObjectSize * m_capacity,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_SHARING_MODE_EXCLUSIVE,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            if (!r) {
                shutdown();
                return false;
            }

            if (!m_stagingBuffer.map()) {
                m_device->getInstance()->error("Failed to map staging buffer for writing");
                shutdown();
                return false;
            }

            m_objects = (u8*)m_stagingBuffer.getPointer();

            return true;
        }

        void UniformBuffer::shutdown() {
            if (m_objects) {
                m_stagingBuffer.unmap();
                m_objects = nullptr;
            }

            m_stagingBuffer.shutdown();
            m_buffer.shutdown();

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
            return m_buffer.get();
        }

        VkDeviceMemory UniformBuffer::getMemory() const {
            return m_buffer.getMemory();
        }

        u32 UniformBuffer::getCapacity() const {
            if (!m_buffer.isValid()) return 0;
            return m_capacity;
        }

        u32 UniformBuffer::getRemaining() const {
            return m_capacity - m_usedCount;
        }

        UniformObject* UniformBuffer::allocate() {
            if (!m_buffer.isValid() || !m_free) return nullptr;
            UniformObject* n = m_free;

            m_free = n->m_next;
            if (m_free) m_free->m_last = nullptr;
            
            n->m_next = m_used;
            if (m_used) m_used->m_last = n;
            m_used = n;

            m_usedCount++;
            return n;
        }

        void UniformBuffer::free(UniformObject* n) {
            if (!m_buffer.isValid() || !m_used || !n || n->m_buffer != this) return;

            if (n->m_last) n->m_last->m_next = n->m_next;
            if (n->m_next) n->m_next->m_last = n->m_last;

            if (n == m_used) m_used = n->m_next;
            
            insertToFreeList(n);
            m_usedCount--;
        }

        void UniformBuffer::submitUpdates(CommandBuffer* cb) {
            if (!m_buffer.isValid() || !m_hasUpdates) return;

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
                    cur->srcOffset = cur->dstOffset = i * m_paddedObjectSize;
                    cur->size = m_paddedObjectSize;
                    doStartNewRange = false;
                    continue;
                }

                cur->size += m_paddedObjectSize;
            }

            vkCmdCopyBuffer(
                cb->get(),
                m_stagingBuffer.get(),
                m_buffer.get(),
                m_copyRanges.size(),
                m_copyRanges.data()
            );

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
            copyData(m_fmt, (u8*)data, m_objects + (idx * m_paddedObjectSize));

            m_hasUpdates = true;
            m_objUpdated[idx] = 1;
            m_minUpdateIdx = utils::min(m_minUpdateIdx, idx);
            m_maxUpdateIdx = utils::max(m_maxUpdateIdx, idx);
        }
        
        u8* UniformBuffer::copyData(const core::DataFormat* fmt, const u8* src, u8* dst) {
            auto& attrs = fmt->getAttributes();
            for (u32 i = 0;i < attrs.size();i++) {
                auto& a = attrs[i];
                switch (a.type) {
                    case dt_int:
                    case dt_float:
                    case dt_uint: {
                        u32* dstE = (u32*)dst;
                        const u32* srcE = (u32*)(src + a.offset);
                        for (u32 c = 0;c < a.elementCount;c++) *(dstE++) = *(srcE++);

                        dst += a.uniformAlignedSize;
                        break;
                    }
                    case dt_vec2i:
                    case dt_vec2f:
                    case dt_vec2ui: {
                        u32* dstE = (u32*)dst;
                        const u32* srcE = (u32*)(src + a.offset);
                        for (u32 c = 0;c < a.elementCount;c++) {
                            *(dstE++) = *(srcE++); // x
                            *(dstE++) = *(srcE++); // y
                        }

                        dst += a.uniformAlignedSize;
                        break;
                    }
                    case dt_vec3i:
                    case dt_vec3f:
                    case dt_vec3ui: {
                        u32* dstE = (u32*)dst;
                        const u32* srcE = (u32*)(src + a.offset);
                        for (u32 c = 0;c < a.elementCount;c++) {
                            *(dstE++) = *(srcE++); // x
                            *(dstE++) = *(srcE++); // y
                            *(dstE++) = *(srcE++); // z
                            *(dstE++) = 0; // 4 bytes padding for vulkan only
                        }

                        dst += a.uniformAlignedSize;
                        break;
                    }
                    case dt_vec4i:
                    case dt_vec4f:
                    case dt_vec4ui: {
                        u32* dstE = (u32*)dst;
                        const u32* srcE = (u32*)(src + a.offset);
                        for (u32 c = 0;c < a.elementCount;c++) {
                            *(dstE++) = *(srcE++); // x
                            *(dstE++) = *(srcE++); // y
                            *(dstE++) = *(srcE++); // z
                            *(dstE++) = *(srcE++); // w
                        }

                        dst += a.uniformAlignedSize;
                        break;
                    }
                    case dt_mat3i:
                    case dt_mat3f:
                    case dt_mat3ui: {
                        u32* dstE = (u32*)dst;
                        const u32* srcE = (u32*)(src + a.offset);
                        for (u32 c = 0;c < a.elementCount;c++) {
                            *(dstE++) = *(srcE++); // mat.x.x
                            *(dstE++) = *(srcE++); // mat.x.y
                            *(dstE++) = *(srcE++); // mat.x.z
                            *(dstE++) = 0; // 4 bytes padding for vulkan only
                            *(dstE++) = *(srcE++); // mat.y.x
                            *(dstE++) = *(srcE++); // mat.y.y
                            *(dstE++) = *(srcE++); // mat.y.z
                            *(dstE++) = 0; // 4 bytes padding for vulkan only
                            *(dstE++) = *(srcE++); // mat.z.x
                            *(dstE++) = *(srcE++); // mat.z.y
                            *(dstE++) = *(srcE++); // mat.z.z
                            *(dstE++) = 0; // 4 bytes padding for vulkan only
                        }

                        dst += a.uniformAlignedSize;
                        break;
                    }
                    case dt_mat4i:
                    case dt_mat4f:
                    case dt_mat4ui: {
                        u32* dstE = (u32*)dst;
                        const u32* srcE = (u32*)(src + a.offset);
                        for (u32 c = 0;c < a.elementCount;c++) {
                            *(dstE++) = *(srcE++); // mat.x.x
                            *(dstE++) = *(srcE++); // mat.x.y
                            *(dstE++) = *(srcE++); // mat.x.z
                            *(dstE++) = *(srcE++); // mat.x.w
                            *(dstE++) = *(srcE++); // mat.y.x
                            *(dstE++) = *(srcE++); // mat.y.y
                            *(dstE++) = *(srcE++); // mat.y.z
                            *(dstE++) = *(srcE++); // mat.y.w
                            *(dstE++) = *(srcE++); // mat.z.x
                            *(dstE++) = *(srcE++); // mat.z.y
                            *(dstE++) = *(srcE++); // mat.z.z
                            *(dstE++) = *(srcE++); // mat.z.w
                            *(dstE++) = *(srcE++); // mat.w.x
                            *(dstE++) = *(srcE++); // mat.w.y
                            *(dstE++) = *(srcE++); // mat.w.z
                            *(dstE++) = *(srcE++); // mat.w.w
                        }

                        dst += a.uniformAlignedSize;
                        break;
                    }
                    case dt_struct: {
                        dst = copyData(a.formatRef, src + a.offset, dst);
                        break;
                    }
                    case dt_enum_count: break;
                }
            }

            return dst;
        }



        //
        // UniformObject
        //

        UniformObject::UniformObject() {
            m_buffer = nullptr;
            m_index = 0;
        }

        UniformObject::~UniformObject() {
        }

        UniformBuffer* UniformObject::getBuffer() const {
            return m_buffer;
        }
        
        void UniformObject::getRange(u32* offset, u32* size) const {
            u32 sz = m_buffer->m_paddedObjectSize;
            if (offset) *offset = m_index * sz;
            if (size) *size = sz;
        }

        void UniformObject::free() {
            m_buffer->free(this);
        }



        //
        // UniformBufferFactory
        //

        UniformBufferFactory::UniformBufferFactory(LogicalDevice* device, u32 maxObjectsPerBuffer) {
            m_device = device;
            m_maxObjectsPerBuffer = maxObjectsPerBuffer;
        }

        UniformBufferFactory::~UniformBufferFactory() {
            freeAll();
        }

        void UniformBufferFactory::freeAll() {
            m_formats.clear();
            m_buffers.each([](Array<UniformBuffer*>& arr) {
                arr.each([](UniformBuffer* buf) {
                    delete buf;
                });
            });
            m_buffers.clear();
        }

        UniformObject* UniformBufferFactory::allocate(core::DataFormat* fmt) {
            i32 idx = m_formats.findIndex([fmt](core::DataFormat* f) {
                return f->isEqualTo(fmt);
            });

            if (idx == -1) {
                UniformBuffer* buf = new UniformBuffer(m_device, fmt, m_maxObjectsPerBuffer);
                if (!buf->init()) {
                    delete buf;
                    return nullptr;
                }

                m_formats.push(fmt);
                m_buffers.push({ buf });

                return buf->allocate();
            }

            Array<UniformBuffer*>& arr = m_buffers[u32(idx)];
            UniformBuffer* buf = arr.find([](UniformBuffer* b) {
                return b->getRemaining() > 0;
            });

            if (buf) return buf->allocate();
            
            buf = new UniformBuffer(m_device, fmt, m_maxObjectsPerBuffer);
            if (!buf->init()) {
                delete buf;
                return nullptr;
            }

            arr.push(buf);
            return buf->allocate();
        }
    };
};