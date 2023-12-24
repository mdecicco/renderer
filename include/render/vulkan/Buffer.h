#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;

        class Buffer {
            public:
                Buffer(LogicalDevice* device);
                ~Buffer();

                LogicalDevice* getDevice() const;
                VkBuffer get() const;
                VkDeviceMemory getMemory() const;
                VkBufferUsageFlags getUsage() const;
                VkSharingMode getSharingMode() const;
                VkMemoryPropertyFlags getMemoryFlags() const;
                bool getRange(u64 offset, u64 size, VkMappedMemoryRange& range) const;
                u64 getSize() const;
                bool isValid() const;

                bool map();
                bool flush(u64 offset = 0, u64 size = VK_WHOLE_SIZE);
                bool write(const void* src, u64 offset, u64 size);
                bool read(u64 offset, u64 size, void* dst, bool fetchFromDevice = true);
                bool fetch(u64 offset, u64 size);
                void* getPointer(u64 offset = 0);
                void unmap();

                bool init(u64 size, VkBufferUsageFlags usage, VkSharingMode sharingMode, VkMemoryPropertyFlags memoryFlags);
                void shutdown();
            
            protected:
                LogicalDevice* m_device;
                u64 m_size;
                VkBuffer m_buffer;
                VkDeviceMemory m_memory;
                VkBufferUsageFlags m_usage;
                VkSharingMode m_sharingMode;
                VkMemoryPropertyFlags m_memoryFlags;
                void* m_mappedMemory;
        };
    };
};