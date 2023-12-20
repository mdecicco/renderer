#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        class SwapChain;

        class RenderPass {
            public:
                struct attachment {
                    VkAttachmentDescription desc;
                    VkAttachmentReference ref;
                };

                // Sets up the render pass for the swap chain
                RenderPass(SwapChain* swapChain);
                ~RenderPass();

                LogicalDevice* getDevice() const;
                const Array<attachment>& getAttachments() const;
                VkRenderPass get() const;

                bool init();
                bool recreate();
                void shutdown();
            
            protected:
                LogicalDevice* m_device;

                VkRenderPass m_renderPass;
                Array<attachment> m_attachments;
                Array<VkAttachmentDescription> m_attachmentDescs;
                Array<VkAttachmentReference> m_attachmentRefs;
                Array<VkSubpassDescription> m_subpasses;
                Array<VkSubpassDependency> m_subpassDeps;
        };
    };
};