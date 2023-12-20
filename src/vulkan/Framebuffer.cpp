#include <render/vulkan/Framebuffer.h>
#include <render/vulkan/RenderPass.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Texture.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/Format.h>

#include <utils/Array.hpp>

namespace render {
    namespace vulkan {
        Framebuffer::Framebuffer(RenderPass* renderPass) {
            m_renderPass = renderPass;
            m_framebuffer = VK_NULL_HANDLE;
        }

        Framebuffer::~Framebuffer() {
            shutdown();
        }

        VkFramebuffer Framebuffer::get() const {
            return m_framebuffer;
        }

        const Array<Framebuffer::attachment>& Framebuffer::getAttachments() const {
            return m_attachments;
        }

        void Framebuffer::setClearColor(u32 attachmentIdx, const vec4f& clearColor) {
            auto& v = m_attachments[attachmentIdx].clearValue.color.float32;
            v[0] = clearColor.x;
            v[1] = clearColor.y;
            v[2] = clearColor.z;
            v[3] = clearColor.w;
        }

        void Framebuffer::setClearColor(u32 attachmentIdx, const vec4ui& clearColor) {
            auto& v = m_attachments[attachmentIdx].clearValue.color.uint32;
            v[0] = clearColor.x;
            v[1] = clearColor.y;
            v[2] = clearColor.z;
            v[3] = clearColor.w;
        }

        void Framebuffer::setClearColor(u32 attachmentIdx, const vec4i& clearColor) {
            auto& v = m_attachments[attachmentIdx].clearValue.color.int32;
            v[0] = clearColor.x;
            v[1] = clearColor.y;
            v[2] = clearColor.z;
            v[3] = clearColor.w;
        }

        void Framebuffer::setClearDepthStencil(u32 attachmentIdx, f32 clearDepth, u32 clearStencil) {
            m_attachments[attachmentIdx].clearValue.depthStencil.depth = clearDepth;
            m_attachments[attachmentIdx].clearValue.depthStencil.stencil = clearStencil;
        }

        Framebuffer::attachment& Framebuffer::attach(VkImageView view, VkFormat format) {
            m_attachments.push({
                view,
                format,
                { 0, 0, 0, 0 }
            });

            return m_attachments.last();
        }

        Framebuffer::attachment& Framebuffer::attach(Texture* texture) {
            m_attachments.push({
                texture->getView(),
                texture->getFormat(),
                { 0, 0, 0, 0 }
            });

            return m_attachments.last();
        }

        bool Framebuffer::init(const vec2ui& dimensions) {
            if (m_framebuffer || m_attachments.size() == 0) return false;

            VkImageView views[16] = {};
            for (u32 i = 0;i < m_attachments.size();i++) {
                views[i] = m_attachments[i].view;
            }

            VkFramebufferCreateInfo fbi = {};
            fbi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbi.renderPass = m_renderPass->get();
            fbi.attachmentCount = m_attachments.size();
            fbi.pAttachments = views;
            fbi.width = dimensions.x;
            fbi.height = dimensions.y;
            fbi.layers = 1;

            LogicalDevice* dev = m_renderPass->getDevice();

            if (vkCreateFramebuffer(dev->get(), &fbi, dev->getInstance()->getAllocator(), &m_framebuffer) != VK_SUCCESS) {
                dev->getInstance()->error(String::Format("Failed to create framebuffer"));
                shutdown();
                return false;
            }

            return true;
        }

        void Framebuffer::shutdown() {
            if (m_framebuffer) {
                LogicalDevice* dev = m_renderPass->getDevice();
                vkDestroyFramebuffer(dev->get(), m_framebuffer, dev->getInstance()->getAllocator());
                m_framebuffer = VK_NULL_HANDLE;
            }
        }
    };
};