#include <render/vulkan/Texture.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/Format.h>

namespace render {
    namespace vulkan {
        Texture::Texture(LogicalDevice* device) : m_stagingBuffer(device) {
            m_device = device;
            m_format = VK_FORMAT_UNDEFINED;
            m_formatInfo = &getFormatInfo(m_format);
            m_mipLevels = 1;
            m_depth = 1;
            m_arrayLayerCount = 1;
            m_dimensions = vec2ui(0, 0);
            m_image = VK_NULL_HANDLE;
            m_memory = VK_NULL_HANDLE;
            m_view = VK_NULL_HANDLE;
            m_sampler = VK_NULL_HANDLE;
        }

        Texture::~Texture() {
            shutdown();
        }

        LogicalDevice* Texture::getDevice() const {
            return m_device;
        }

        Buffer* Texture::getStagingBuffer() {
            if (!m_stagingBuffer.isValid()) return nullptr;

            return &m_stagingBuffer;
        }

        const Buffer* Texture::getStagingBuffer() const {
            if (!m_stagingBuffer.isValid()) return nullptr;
            
            return &m_stagingBuffer;
        }

        VkImageType Texture::getType() const {
            return m_type;
        }

        VkFormat Texture::getFormat() const {
            return m_format;
        }

        u32 Texture::getBytesPerPixel() const {
            return m_formatInfo->size;
        }
        
        u32 Texture::getChannelCount() const {
            return m_formatInfo->channelCount;
        }
        
        u32 Texture::getMipLevelCount() const {
            return m_mipLevels;
        }

        u32 Texture::getDepth() const {
            return m_depth;
        }

        u32 Texture::getArrayLayerCount() const {
            return m_arrayLayerCount;
        }

        vec2ui Texture::getDimensions() const {
            return m_dimensions;
        }

        VkImage Texture::get() const {
            return m_image;
        }
        
        VkImageView Texture::getView() const {
            return m_view;
        }

        VkSampler Texture::getSampler() const {
            return m_sampler;
        }

        bool Texture::init(
            u32 width,
            u32 height,
            VkFormat format,
            VkImageType type,
            u32 mipLevels,
            u32 depth,
            u32 arrayLayers,
            VkImageUsageFlags usage,
            VkImageLayout layout
        ) {
            if (m_image) return false;

            m_type = type;
            m_layout = layout;
            m_format = format;
            m_formatInfo = &getFormatInfo(format);
            m_mipLevels = mipLevels;
            m_depth = depth;
            m_arrayLayerCount = arrayLayers;
            m_dimensions = vec2ui(width, height);

            VkImageCreateInfo ii = {};
            ii.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ii.imageType = m_type;
            ii.format = m_format;
            ii.tiling = VK_IMAGE_TILING_OPTIMAL;
            ii.initialLayout = m_layout;
            ii.usage = usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            ii.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ii.samples = VK_SAMPLE_COUNT_1_BIT;
            ii.extent.width = m_dimensions.x;
            ii.extent.height = m_dimensions.y;
            ii.extent.depth = m_depth;
            ii.mipLevels = m_mipLevels;
            ii.arrayLayers = m_arrayLayerCount;

            if (vkCreateImage(m_device->get(), &ii, m_device->getInstance()->getAllocator(), &m_image) != VK_SUCCESS) {
                m_device->getInstance()->error("Call to vkCreateImage for texture failed");
                shutdown();
                return false;
            }

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(m_device->get(), m_image, &memReqs);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReqs.size;
            if (!m_device->getPhysicalDevice()->getMemoryTypeIndex(memReqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocInfo.memoryTypeIndex)) {
                m_device->getInstance()->error("Failed to find correct memory type for image");
                shutdown();
                return false;
            }

            if (vkAllocateMemory(m_device->get(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
                shutdown();
                return false;
            }

            vkBindImageMemory(m_device->get(), m_image, m_memory, 0);

            VkImageViewCreateInfo vi = {};
            vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            vi.image = m_image;
            vi.viewType = VK_IMAGE_VIEW_TYPE_2D; // todo
            vi.format = m_format;
            vi.subresourceRange.baseMipLevel = 0;
            vi.subresourceRange.levelCount = m_mipLevels;
            vi.subresourceRange.baseArrayLayer = 0;
            vi.subresourceRange.layerCount = m_arrayLayerCount;

            switch (format) {
                case VK_FORMAT_D16_UNORM:
                case VK_FORMAT_X8_D24_UNORM_PACK32:
                case VK_FORMAT_D32_SFLOAT:
                case VK_FORMAT_D16_UNORM_S8_UINT:
                case VK_FORMAT_D24_UNORM_S8_UINT:
                case VK_FORMAT_D32_SFLOAT_S8_UINT: {
                    vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    break;
                }
                default: {
                    vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    break;
                }
            }

            if (vkCreateImageView(m_device->get(), &vi, m_device->getInstance()->getAllocator(), &m_view) != VK_SUCCESS) {
                m_device->getInstance()->error("Call to vkCreateImageView for texture failed");
                shutdown();
                return false;
            }

            return true;
        }

        bool Texture::initSampler() {
            VkSamplerCreateInfo si = {};
            si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            si.magFilter = VK_FILTER_LINEAR;
            si.minFilter = VK_FILTER_LINEAR;
            si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            si.anisotropyEnable = VK_TRUE;
            si.maxAnisotropy = m_device->getPhysicalDevice()->getProperties().limits.maxSamplerAnisotropy;
            si.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            si.unnormalizedCoordinates = VK_FALSE;
            si.compareEnable = VK_FALSE;
            si.compareOp = VK_COMPARE_OP_ALWAYS;
            si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            si.mipLodBias = 0.0f;
            si.minLod = 0.0f;
            si.maxLod = 0.0f;

            if (vkCreateSampler(m_device->get(), &si, m_device->getInstance()->getAllocator(), &m_sampler) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to create sampler for texture");
                return false;
            }

            return true;
        }

        bool Texture::initStagingBuffer() {
            if (!m_image) return false;

            bool r;
            
            r = m_stagingBuffer.init(
                m_dimensions.x * m_dimensions.y * m_formatInfo->size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_SHARING_MODE_EXCLUSIVE,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            if (!r || !m_stagingBuffer.map()) {
                shutdown();
                return false;
            }

            return true;
        }

        void Texture::shutdown() {
            if (m_sampler) {
                vkDestroySampler(m_device->get(), m_sampler, m_device->getInstance()->getAllocator());
                m_sampler = VK_NULL_HANDLE;
            }

            if (m_view) {
                vkDestroyImageView(m_device->get(), m_view, m_device->getInstance()->getAllocator());
                m_view = VK_NULL_HANDLE;
            }

            if (m_image) {
                vkDestroyImage(m_device->get(), m_image, m_device->getInstance()->getAllocator());
                m_image = VK_NULL_HANDLE;
            }

            if (m_memory) {
                vkFreeMemory(m_device->get(), m_memory, m_device->getInstance()->getAllocator());
                m_memory = VK_NULL_HANDLE;
            }

            shutdownStagingBuffer();

            m_format = VK_FORMAT_UNDEFINED;
            m_formatInfo = &getFormatInfo(m_format);
            m_mipLevels = 1;
            m_depth = 1;
            m_arrayLayerCount = 1;
            m_dimensions = vec2ui(0, 0);
        }

        void Texture::shutdownStagingBuffer() {
            m_stagingBuffer.shutdown();
        }

        bool Texture::setLayout(CommandBuffer* cb, VkImageLayout layout) {
            VkImageMemoryBarrier b = {};
            b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.oldLayout = m_layout;
            b.newLayout = layout;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.image = m_image;
            b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            b.subresourceRange.baseMipLevel = 0;
            b.subresourceRange.levelCount = m_mipLevels;
            b.subresourceRange.baseArrayLayer = 0;
            b.subresourceRange.layerCount = m_arrayLayerCount;

            VkPipelineStageFlags srcStage, dstStage;

            if (m_layout == VK_IMAGE_LAYOUT_UNDEFINED && layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                b.srcAccessMask = 0;
                b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else if (m_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else if (m_layout == VK_IMAGE_LAYOUT_UNDEFINED && layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                b.srcAccessMask = 0;
                b.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            } else if (m_layout == VK_IMAGE_LAYOUT_UNDEFINED && layout == VK_IMAGE_LAYOUT_GENERAL) {
                b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
                dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else {
                m_device->getInstance()->error("Invalid layout transition");
                return false;
            }

            if (layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                if (
                    m_format == VK_FORMAT_D16_UNORM_S8_UINT ||
                    m_format == VK_FORMAT_D24_UNORM_S8_UINT ||
                    m_format == VK_FORMAT_D32_SFLOAT_S8_UINT
                ) {
                    b.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            } else {
                b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            }

            vkCmdPipelineBarrier(
                cb->get(),
                srcStage, dstStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &b
            );

            m_layout = layout;

            return true;
        }
        
        void Texture::flushPixels(CommandBuffer* cb) {
            if (!m_stagingBuffer.isValid()) return;

            VkBufferImageCopy region = {};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = m_arrayLayerCount;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = {
                m_dimensions.x,
                m_dimensions.y,
                m_depth
            };

            vkCmdCopyBufferToImage(
                cb->get(),
                m_stagingBuffer.get(),
                m_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );
        }
    };
};