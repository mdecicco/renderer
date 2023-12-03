#pragma once
#include <render/types.h>

#include <utils/Array.h>
#include <vulkan/vulkan.h>

namespace render {
    namespace core {
        class DataFormat;
    };

    namespace vulkan {
        class LogicalDevice;
        class Vertices;

        class VertexBuffer {
            public:
                VertexBuffer(LogicalDevice* device, core::DataFormat* fmt, u32 vertexCapacity);
                ~VertexBuffer();

                bool init();
                void shutdown();

                LogicalDevice* getDevice() const;
                core::DataFormat* getFormat() const;
                VkBuffer getBuffer() const;
                VkDeviceMemory getMemory() const;
                u32 getCapacity() const;
                u32 getCurrentMaximumBlockSize() const;

                Vertices* allocate(u32 count);
                void free(Vertices* verts);
            
            private:
                friend class Vertices;

                struct node {
                    Vertices* verts;
                    u32 offset;
                    u32 size;
                    node* last;
                    node* next;
                };

                void recalculateMaximumBlockSize();
                void resetNodes();
                node* getNode(u32 vcount);
                void recycleNode(node* n);
                void insertToFreeList(node* n);

                LogicalDevice* m_device;
                core::DataFormat* m_fmt;
                u32 m_capacity;
                u32 m_currentMaximumBlockSize;
                u32 m_nodeCount;
                u32 m_memoryMapRefCount;

                VkBuffer m_buffer;
                VkDeviceMemory m_memory;

                node* m_freeBlocks;
                node* m_usedBlocks;
                node* m_unusedNodes;
                node* m_nodes;
        };

        class Vertices {
            public:
                u32 getOffset() const;
                u32 getByteOffset() const;
                u32 getSize() const;
                u32 getCount() const;
                VertexBuffer* getBuffer() const;
                void free();

                bool beginUpdate(bool willReadData = false);
                
                void write(void* data, u32 offset, u32 count);
                
                // will dereference null pointer if beginUpdate is not called or
                // if a beginUpdate failure is ignored
                template <typename VertexTp>
                VertexTp& at(u32 idx) { return ((VertexTp*)m_mappedMemory)[idx]; }

                bool commitUpdate();

            private:
                friend class VertexBuffer;

                Vertices(VertexBuffer* buf, core::DataFormat* fmt, VertexBuffer::node* n);
                ~Vertices();

                VertexBuffer* m_buffer;
                core::DataFormat* m_fmt;
                VertexBuffer::node* m_node;
                VkMappedMemoryRange m_range;
                void* m_mappedMemory;
        };

        class VertexBufferFactory {
            public:
                VertexBufferFactory(LogicalDevice* device, u32 maxBufferCapacity);
                ~VertexBufferFactory();

                void freeAll();

                Vertices* allocate(core::DataFormat* fmt, u32 count);
            
            private:
                LogicalDevice* m_device;
                u32 m_maxBufCapacity;

                utils::Array<core::DataFormat*> m_formats;
                utils::Array<utils::Array<VertexBuffer*>> m_buffers;
        };
    };
};