#include <render/vulkan/RenderPass.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Instance.h>

#include <utils/Array.hpp>

namespace render {
    namespace vulkan {
        RenderPass::RenderPass(SwapChain* swapChain) {
            m_device = swapChain->getDevice();
            m_renderPass = VK_NULL_HANDLE;

            // color attachment
            m_attachmentDescs.push({});
            auto& ca = m_attachmentDescs.last();
            ca.format = swapChain->getFormat();
            ca.samples = VK_SAMPLE_COUNT_1_BIT;
            ca.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            ca.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            ca.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            ca.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            ca.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            m_attachmentRefs.push({});
            auto& car = m_attachmentRefs.last();
            car.attachment = 0;
            car.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // depth attachment
            m_attachmentDescs.push({});
            auto& da = m_attachmentDescs.last();
            da.format = VK_FORMAT_D32_SFLOAT;
            da.samples = VK_SAMPLE_COUNT_1_BIT;
            da.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            da.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            da.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            da.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            da.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            da.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            m_attachmentRefs.push({});
            auto& dar = m_attachmentRefs.last();
            dar.attachment = 1;
            dar.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            m_subpasses.push({});
            auto& sd = m_subpasses.last();
            sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            sd.colorAttachmentCount = 1;
            sd.pColorAttachments = &m_attachmentRefs[0]; 
            sd.pDepthStencilAttachment = &m_attachmentRefs[1];

            m_subpassDeps.push({});
            auto& dep = m_subpassDeps.last();
            dep.srcSubpass = VK_SUBPASS_EXTERNAL;
            dep.dstSubpass = 0;
            dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dep.srcAccessMask = 0;
            dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        RenderPass::~RenderPass() {
            shutdown();
        }

        LogicalDevice* RenderPass::getDevice() const {
            return m_device;
        }

        VkRenderPass RenderPass::get() const {
            return m_renderPass;
        }


        bool RenderPass::init() {
            VkRenderPassCreateInfo rpi = {};
            rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            rpi.attachmentCount = m_attachmentDescs.size();
            rpi.pAttachments = m_attachmentDescs.data();
            rpi.subpassCount = m_subpasses.size();
            rpi.pSubpasses = m_subpasses.data();
            rpi.dependencyCount = m_subpassDeps.size();
            rpi.pDependencies = m_subpassDeps.data();

            if (vkCreateRenderPass(m_device->get(), &rpi, m_device->getInstance()->getAllocator(), &m_renderPass) != VK_SUCCESS) {
                shutdown();
                return false;
            }

            return true;
        }

        void RenderPass::shutdown() {
            if (m_renderPass) {
                vkDestroyRenderPass(m_device->get(), m_renderPass, m_device->getInstance()->getAllocator());
                m_renderPass = VK_NULL_HANDLE;
            }
        }
        
        bool RenderPass::recreate() {
            shutdown();
            return init();
        }
    };
};