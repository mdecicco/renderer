#include <render/vulkan/RenderPass.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/core/FrameManager.h>

namespace render {
    namespace vulkan {
        RenderPass::RenderPass(SwapChain* swapChain) {
            m_swapChain = swapChain;
            m_renderPass = VK_NULL_HANDLE;
            swapChain->onRenderPassCreated(this);
        }

        RenderPass::~RenderPass() {
            shutdown();
            m_swapChain->onRenderPassDestroyed(this);
        }

        SwapChain* RenderPass::getSwapChain() const {
            return m_swapChain;
        }
        
        core::FrameManager* RenderPass::getFrameManager() const {
            return m_frames;
        }

        VkRenderPass RenderPass::get() const {
            return m_renderPass;
        }


        bool RenderPass::init() {
            VkAttachmentDescription at[2] = {{}, {}};
            VkAttachmentReference ar[2] = {{}, {}};

            // color attachment
            at[0].format = m_swapChain->getFormat();
            at[0].samples = VK_SAMPLE_COUNT_1_BIT;
            at[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            at[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            at[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            at[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            at[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            at[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            ar[0].attachment = 0;
            ar[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // depth attachment
            at[1].format = VK_FORMAT_D32_SFLOAT;
            at[1].samples = VK_SAMPLE_COUNT_1_BIT;
            at[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            at[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            at[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            at[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            at[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            at[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            ar[1].attachment = 1;
            ar[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription sd = {};
            sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            sd.colorAttachmentCount = 1;
            sd.pColorAttachments = &ar[0]; 
            sd.pDepthStencilAttachment = &ar[1];

            VkSubpassDependency dep = {};
            dep.srcSubpass = VK_SUBPASS_EXTERNAL;
            dep.dstSubpass = 0;
            dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dep.srcAccessMask = 0;
            dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo rpi = {};
            rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            rpi.attachmentCount = 2;
            rpi.pAttachments = at;
            rpi.subpassCount = 1;
            rpi.pSubpasses = &sd;
            rpi.dependencyCount = 1;
            rpi.pDependencies = &dep;

            LogicalDevice* dev = m_swapChain->getDevice();
            if (vkCreateRenderPass(dev->get(), &rpi, dev->getInstance()->getAllocator(), &m_renderPass) != VK_SUCCESS) {
                shutdown();
                return false;
            }

            m_frames = new core::FrameManager(this, m_swapChain->getImages().size());
            if (!m_frames->init()) {
                shutdown();
                return false;
            }

            m_frames->subscribeLogger(dev->getInstance());

            return true;
        }

        void RenderPass::shutdown() {
            if (m_frames) {
                delete m_frames;
                m_frames = nullptr;
            }
            
            if (m_renderPass) {
                LogicalDevice* dev = m_swapChain->getDevice();
                vkDestroyRenderPass(dev->get(), m_renderPass, dev->getInstance()->getAllocator());
                m_renderPass = VK_NULL_HANDLE;
            }
        }
        
        bool RenderPass::recreate() {
            shutdown();
            return init();
        }

        core::FrameContext* RenderPass::getFrame() {
            if (!m_frames) return nullptr;

            return m_frames->getFrame();
        }

        void RenderPass::releaseFrame(core::FrameContext* frame) {
            if (!m_frames) return;

            m_frames->releaseFrame(frame);
        }
    };
};