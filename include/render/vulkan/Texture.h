#pragma once
#include <render/types.h>
#include <render/vulkan/Buffer.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        class CommandBuffer;
        struct VulkanFormatInfo;

        class Texture {
            public:
                Texture(LogicalDevice* device);
                ~Texture();

                LogicalDevice* getDevice() const;
                Buffer* getStagingBuffer();
                const Buffer* getStagingBuffer() const;
                VkImageType getType() const;
                VkFormat getFormat() const;
                u32 getBytesPerPixel() const;
                u32 getChannelCount() const;
                u32 getMipLevelCount() const;
                u32 getDepth() const;
                u32 getArrayLayerCount() const;
                vec2ui getDimensions() const;
                VkImage get() const;
                VkImageView getView() const;
                VkSampler getSampler() const;

                bool init(
                    u32 width,
                    u32 height,
                    VkFormat format,
                    VkImageType type = VK_IMAGE_TYPE_2D,
                    u32 mipLevels = 1,
                    u32 depth = 1,
                    u32 arrayLayers = 1,
                    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT,
                    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED
                );
                bool initSampler();
                bool initStagingBuffer();
                void shutdown();
                void shutdownStagingBuffer();

                bool setLayout(CommandBuffer* cb, VkImageLayout layout);
                void flushPixels(CommandBuffer* cb);

            protected:
                LogicalDevice* m_device;
                VkImageType m_type;
                VkImageLayout m_layout;
                VkFormat m_format;
                const VulkanFormatInfo* m_formatInfo;
                u32 m_mipLevels;
                u32 m_depth;
                u32 m_arrayLayerCount;
                vec2ui m_dimensions;

                Buffer m_stagingBuffer;
                VkImage m_image;
                VkDeviceMemory m_memory;
                VkImageView m_view;
                VkSampler m_sampler;
        };
    };
};