#include <render/IWithRendering.h>

#include <utils/Array.hpp>
#include <utils/String.h>
#include <utils/Window.h>

#include <render/vulkan/Instance.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Surface.h>
#include <render/vulkan/SwapChainSupport.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/ShaderCompiler.h>
#include <render/vulkan/Pipeline.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/Queue.h>

namespace render {
    IWithRendering::IWithRendering() : utils::IWithLogging("Render") {
        m_window = nullptr;
        m_instance = nullptr;
        m_physicalDevice = nullptr;
        m_logicalDevice = nullptr;
        m_surface = nullptr;
        m_swapChain = nullptr;
        m_shaderCompiler = nullptr;
        m_swapChainReady = VK_NULL_HANDLE;
        m_renderComplete = VK_NULL_HANDLE;
        m_frameFence = VK_NULL_HANDLE;

        m_initialized = false;
        m_frameStarted = false;
        m_currentImageIndex = 0;
    }

    IWithRendering::~IWithRendering() {
        shutdownRendering();
    }
    
    const vulkan::PhysicalDevice* IWithRendering::choosePhysicalDevice(const utils::Array<vulkan::PhysicalDevice>& devices) {
        const vulkan::PhysicalDevice* gpu = nullptr;
        render::vulkan::SwapChainSupport swapChainSupport;

        for (render::u32 i = 0;i < devices.size() && !gpu;i++) {
            if (!devices[i].isDiscrete()) continue;
            if (!devices[i].isExtensionAvailable(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;

            if (!devices[i].getSurfaceSwapChainSupport(m_surface, &swapChainSupport)) continue;
            if (!swapChainSupport.isValid()) continue;

            if (!swapChainSupport.hasFormat(VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) continue;
            if (!swapChainSupport.hasPresentMode(VK_PRESENT_MODE_FIFO_KHR)) continue;

            auto& capabilities = swapChainSupport.getCapabilities();
            if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < 3) continue;

            gpu = &devices[i];
        }

        return gpu;
    }

    bool IWithRendering::setupInstance(vulkan::Instance* instance) {
        return true;
    }
    
    bool IWithRendering::setupDevice(vulkan::LogicalDevice* device) {
        return device->init(true, false, false, m_surface);
    }
    
    bool IWithRendering::setupSwapchain(vulkan::SwapChain* swapChain, const vulkan::SwapChainSupport& support) {
        return swapChain->init(
            m_surface,
            m_logicalDevice,
            support,
            VK_FORMAT_B8G8R8A8_SRGB,
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            VK_PRESENT_MODE_FIFO_KHR,
            3
        );
    }

    bool IWithRendering::setupShaderCompiler(vulkan::ShaderCompiler* shaderCompiler) {
        return true;
    }

    bool IWithRendering::begin(vulkan::CommandBuffer* cb, vulkan::Pipeline* pipeline) {
        if (m_frameStarted || !m_initialized) return false;

        if (!waitForFrameEnd()) return false;

        if (vkAcquireNextImageKHR(m_logicalDevice->get(), m_swapChain->get(), UINT64_MAX, m_swapChainReady, nullptr, &m_currentImageIndex) != VK_SUCCESS) {
            return false;
        }

        if (!cb->reset()) return false;
        if (!cb->begin()) return false;

        cb->beginRenderPass(pipeline, { 0.25f, 0.25f, 0.25f, 1.0f }, m_currentImageIndex);
        m_frameStarted = true;

        return true;
    }

    bool IWithRendering::end(vulkan::CommandBuffer* cb, vulkan::Pipeline* pipeline) {
        if (!m_frameStarted) return false;

        cb->endRenderPass();
        if (!cb->end()) return false;

        VkPipelineStageFlags sf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkCommandBuffer buf = cb->get();
        VkSwapchainKHR swap = m_swapChain->get();

        VkSubmitInfo si = {};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = &m_swapChainReady;
        si.pWaitDstStageMask = &sf;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &buf;
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = &m_renderComplete;

        if (vkQueueSubmit(m_logicalDevice->getGraphicsQueue()->get(), 1, &si, m_frameFence) != VK_SUCCESS) {
            // not totally catastrophic
            return true;
        }
        m_frameStarted = false;

        VkPresentInfoKHR pi = {};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = &m_renderComplete;
        pi.swapchainCount = 1;
        pi.pSwapchains = &swap;
        pi.pImageIndices = &m_currentImageIndex;

        if (vkQueuePresentKHR(m_logicalDevice->getGraphicsQueue()->get(), &pi) != VK_SUCCESS) {
            // not totally catastrophic
            return true;
        }

        return true;
    }

    bool IWithRendering::waitForFrameEnd() {
        if (m_frameStarted || !m_initialized) return false;
        if (vkWaitForFences(m_logicalDevice->get(), 1, &m_frameFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) return false;
        if (vkResetFences(m_logicalDevice->get(), 1, &m_frameFence) != VK_SUCCESS) return false;
        return true;
    }

    utils::Window* IWithRendering::getWindow() const {
        return m_window;
    }

    vulkan::Instance* IWithRendering::getInstance() const {
        return m_instance;
    }

    vulkan::PhysicalDevice* IWithRendering::getPhysicalDevice() const {
        return m_physicalDevice;
    }

    vulkan::LogicalDevice* IWithRendering::getLogicalDevice() const {
        return m_logicalDevice;
    }

    vulkan::Surface* IWithRendering::getSurface() const {
        return m_surface;
    }

    vulkan::SwapChain* IWithRendering::getSwapChain() const {
        return m_swapChain;
    }
    
    vulkan::ShaderCompiler* IWithRendering::getShaderCompiler() const {
        return m_shaderCompiler;
    }

    bool IWithRendering::initRendering(utils::Window* win) {
        if (m_initialized) return false;

        m_instance = new vulkan::Instance();
        m_instance->subscribeLogger(this);
        if (!setupInstance(m_instance)) {
            fatal("Client instance setup failed");
            shutdownRendering();
            return false;
        }

        if (!m_instance->initialize()) {
            fatal("Instance initialization failed");
            shutdownRendering();
            return false;
        }

        m_surface = new vulkan::Surface(m_instance, win);
        if (!m_surface->init()) {
            fatal("Surface initialization failed");
            shutdownRendering();
            return false;
        }

        auto devices = vulkan::PhysicalDevice::list(m_instance);
        if (devices.size() == 0) {
            fatal("No supported physical device exists");
            shutdownRendering();
            return false;
        }

        const vulkan::PhysicalDevice* selectedDevice = choosePhysicalDevice(devices);
        if (!selectedDevice) {
            fatal("No physical device was specified");
            shutdownRendering();
            return false;
        }

        m_physicalDevice = new vulkan::PhysicalDevice(*selectedDevice);
        m_logicalDevice = new vulkan::LogicalDevice(m_physicalDevice);

        if (!m_logicalDevice->enableExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
            fatal("Selected device '%s' does not support the swapchain extension", m_physicalDevice->getProperties().deviceName);
            shutdownRendering();
            return false;
        }

        if (!setupDevice(m_logicalDevice)) {
            fatal("Client setup for '%s' failed.", m_physicalDevice->getProperties().deviceName);
            shutdownRendering();
            return false;
        }

        vulkan::SwapChainSupport scSupport;
        if (!m_physicalDevice->getSurfaceSwapChainSupport(m_surface, &scSupport)) {
            fatal("Failed to get swapchain support for '%s'.", m_physicalDevice->getProperties().deviceName);
            shutdownRendering();
            return false;
        }

        m_swapChain = new vulkan::SwapChain();
        if (!setupSwapchain(m_swapChain, scSupport)) {
            fatal("Client setup for swapchain failed.");
            shutdownRendering();
            return false;
        }

        if (!m_swapChain->isValid()) {
            fatal("Swapchain is invalid.");
            shutdownRendering();
            return false;
        }

        m_shaderCompiler = new vulkan::ShaderCompiler();
        m_shaderCompiler->subscribeLogger(this);

        if (!m_shaderCompiler->init()) {
            fatal("Failed to initialize shader compiler.");
            shutdownRendering();
            return false;
        }

        VkSemaphoreCreateInfo si = {};
        si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(m_logicalDevice->get(), &si, m_instance->getAllocator(), &m_swapChainReady) != VK_SUCCESS) {
            fatal("Failed to create swapchain semaphore.");
            shutdownRendering();
            return false;
        }

        if (vkCreateSemaphore(m_logicalDevice->get(), &si, m_instance->getAllocator(), &m_renderComplete) != VK_SUCCESS) {
            fatal("Failed to create render completion semaphore.");
            shutdownRendering();
            return false;
        }

        VkFenceCreateInfo fi = {};
        fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateFence(m_logicalDevice->get(), &fi, m_instance->getAllocator(), &m_frameFence) != VK_SUCCESS) {
            fatal("Failed to create frame fence semaphore.");
            shutdownRendering();
            return false;
        }

        m_initialized = true;
        return true;
    }

    void IWithRendering::shutdownRendering() {
        if (m_frameFence) {
            vkDestroyFence(m_logicalDevice->get(), m_frameFence, m_instance->getAllocator());
            m_frameFence = VK_NULL_HANDLE;
        }

        if (m_renderComplete) {
            vkDestroySemaphore(m_logicalDevice->get(), m_renderComplete, m_instance->getAllocator());
            m_renderComplete = VK_NULL_HANDLE;
        }

        if (m_swapChainReady) {
            vkDestroySemaphore(m_logicalDevice->get(), m_swapChainReady, m_instance->getAllocator());
            m_swapChainReady = VK_NULL_HANDLE;
        }

        if (m_shaderCompiler) {
            delete m_shaderCompiler;
            m_shaderCompiler = nullptr;
        }

        if (m_swapChain) {
            delete m_swapChain;
            m_swapChain = nullptr;
        }

        if (m_logicalDevice) {
            delete m_logicalDevice;
            m_logicalDevice = nullptr;
        }

        if (m_physicalDevice) {
            delete m_physicalDevice;
            m_physicalDevice = nullptr;
        }

        if (m_surface) {
            delete m_surface;
            m_surface = nullptr;
        }

        if (m_instance) {
            delete m_instance;
            m_instance = nullptr;
        }

        m_initialized = false;
    }
};