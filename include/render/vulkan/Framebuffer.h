#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class RenderPass;
        class Texture;

        class Framebuffer {
            public:
                struct attachment {
                    VkImageView view;
                    VkFormat format;
                    VkClearValue clearValue;
                };

                Framebuffer(RenderPass* renderPass);
                ~Framebuffer();

                VkFramebuffer get() const;
                const Array<attachment>& getAttachments() const;

                void setClearColor(u32 attachmentIdx, const vec4f& clearColor);
                void setClearColor(u32 attachmentIdx, const vec4ui& clearColor);
                void setClearColor(u32 attachmentIdx, const vec4i& clearColor);
                void setClearDepthStencil(u32 attachmentIdx, f32 clearDepth, u32 clearStencil = 0);
                void attach(VkImageView view, VkFormat format);
                void attach(Texture* texture);

                bool init();
                void shutdown();
            
            private:
                RenderPass* m_renderPass;

                Array<attachment> m_attachments;
                VkFramebuffer m_framebuffer;
        };
    };
};