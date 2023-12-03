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
        class UniformObject;
        class CommandBuffer;
        class Pipeline;

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

                UniformObject* allocate(u32 bindIndex, Pipeline* pipeline);
                void free(UniformObject* data);

                void submitUpdates(CommandBuffer* cb);
            
            private:
                friend class UniformObject;

                void resetNodes();
                void insertToFreeList(UniformObject* n);
                void updateObject(UniformObject* n, const void* data);

                LogicalDevice* m_device;
                core::DataFormat* m_fmt;
                u32 m_capacity;
                u32 m_usedCount;

                VkDescriptorPool m_pool;
                VkBuffer m_buffer;
                VkDeviceMemory m_memory;
                VkBuffer m_stagingBuffer;
                VkDeviceMemory m_stagingMemory;

                UniformObject* m_free;
                UniformObject* m_used;
                UniformObject* m_nodes;
                u8* m_objects;
                u8* m_objUpdated;
                bool m_hasUpdates;
                u32 m_minUpdateIdx;
                u32 m_maxUpdateIdx;
                utils::Array<VkBufferCopy> m_copyRanges;
        };

        class UniformObject {
            public:
                UniformBuffer* getBuffer() const;
                u32 getBindIndex() const;
                VkDescriptorSet getDescriptorSet() const;
                void free();

                template <typename ObjectTp>
                void set(const ObjectTp& data) { m_buffer->updateObject(this, &data); }

            private:
                friend class UniformBuffer;

                UniformObject();
                ~UniformObject();

                u32 m_bindIdx;
                VkDescriptorSet m_set;
                UniformBuffer* m_buffer;
                u32 m_index;
                UniformObject* m_last;
                UniformObject* m_next;
        };

        class UniformBufferFactory {
            public:
                UniformBufferFactory(LogicalDevice* device, u32 maxBufferObjectCapacity);
                ~UniformBufferFactory();

                void freeAll();

                UniformObject* allocate(core::DataFormat* fmt, u32 bindIndex, Pipeline* pipeline);
            
            private:
                LogicalDevice* m_device;
                u32 m_maxBufObjCapacity;

                utils::Array<core::DataFormat*> m_formats;
                utils::Array<utils::Array<UniformBuffer*>> m_buffers;
        };
    };
};