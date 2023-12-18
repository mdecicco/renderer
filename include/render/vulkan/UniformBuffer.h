#pragma once
#include <render/types.h>
#include <render/vulkan/Buffer.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace core {
        class DataFormat;
    };

    namespace vulkan {
        class LogicalDevice;
        class UniformObject;
        class CommandBuffer;

        class UniformBuffer {
            public:
                UniformBuffer(LogicalDevice* device, core::DataFormat* fmt, u32 objectCapacity);
                ~UniformBuffer();

                bool init();
                void shutdown();

                LogicalDevice* getDevice() const;
                core::DataFormat* getFormat() const;
                VkBuffer getBuffer() const;
                VkDeviceMemory getMemory() const;
                u32 getCapacity() const;
                u32 getRemaining() const;

                UniformObject* allocate();
                void free(UniformObject* data);

                void submitUpdates(CommandBuffer* cb);
            
            private:
                friend class UniformObject;

                void resetNodes();
                void insertToFreeList(UniformObject* n);
                void updateObject(UniformObject* n, const void* data);
                u8* copyData(const core::DataFormat* fmt, const u8* src, u8* dst);

                LogicalDevice* m_device;
                core::DataFormat* m_fmt;
                u32 m_capacity;
                u32 m_usedCount;
                u32 m_paddedObjectSize;
                
                Buffer m_buffer;
                Buffer m_stagingBuffer;

                UniformObject* m_free;
                UniformObject* m_used;
                UniformObject* m_nodes;
                u8* m_objects;
                u8* m_objUpdated;
                bool m_hasUpdates;
                u32 m_minUpdateIdx;
                u32 m_maxUpdateIdx;
                Array<VkBufferCopy> m_copyRanges;
        };

        class UniformObject {
            public:
                UniformBuffer* getBuffer() const;
                void getRange(u32* offset, u32* size) const;
                void free();

                template <typename ObjectTp>
                void set(const ObjectTp& data) { m_buffer->updateObject(this, &data); }

            private:
                friend class UniformBuffer;

                UniformObject();
                ~UniformObject();

                UniformBuffer* m_buffer;
                u32 m_index;
                UniformObject* m_last;
                UniformObject* m_next;
        };

        class UniformBufferFactory {
            public:
                UniformBufferFactory(LogicalDevice* device, u32 maxObjectsPerBuffer);
                ~UniformBufferFactory();

                void freeAll();

                UniformObject* allocate(core::DataFormat* fmt);
            
            private:
                LogicalDevice* m_device;
                u32 m_maxObjectsPerBuffer;

                Array<core::DataFormat*> m_formats;
                Array<Array<UniformBuffer*>> m_buffers;
        };
    };
};