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
#include <render/vulkan/RenderPass.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/Queue.h>
#include <render/vulkan/VertexBuffer.h>
#include <render/vulkan/UniformBuffer.h>
#include <render/vulkan/DescriptorSet.h>
#include <render/core/FrameManager.h>
#include <render/core/FrameContext.h>
#include <render/utils/SimpleDebugDraw.h>
#include <render/utils/ImGui.h>

namespace render {
    IWithRendering::IWithRendering() : ::utils::IWithLogging("Render") {
        m_window = nullptr;
        m_instance = nullptr;
        m_physicalDevice = nullptr;
        m_logicalDevice = nullptr;
        m_surface = nullptr;
        m_swapChain = nullptr;
        m_shaderCompiler = nullptr;
        m_debugDraw = nullptr;
        m_imgui = nullptr;
        m_frames = nullptr;
        m_vboFactory = nullptr;
        m_uboFactory = nullptr;
        m_descriptorFactory = nullptr;

        m_initialized = false;
    }

    IWithRendering::~IWithRendering() {
        shutdownRendering();
    }

    bool IWithRendering::initRendering(::utils::Window* win) {
        if (m_initialized) return false;

        m_window = win;

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

        m_renderPass = new vulkan::RenderPass(m_swapChain);
        if (!m_renderPass->init()) {
            fatal("Failed to initialize builtin render pass for swap chain");
            shutdownRendering();
            return false;
        }

        m_frames = new core::FrameManager(m_swapChain, m_renderPass);
        if (!m_frames->init()) {
            shutdownRendering();
            return false;
        }

        m_frames->subscribeLogger(m_logicalDevice->getInstance());

        m_shaderCompiler = new vulkan::ShaderCompiler();
        m_shaderCompiler->subscribeLogger(this);

        if (!m_shaderCompiler->init()) {
            fatal("Failed to initialize shader compiler.");
            shutdownRendering();
            return false;
        }

        m_vboFactory = new vulkan::VertexBufferFactory(m_logicalDevice, 8096);
        m_uboFactory = new vulkan::UniformBufferFactory(m_logicalDevice, 1024);
        m_descriptorFactory = new vulkan::DescriptorFactory(m_logicalDevice, 256);

        m_window->subscribe(this);

        m_initialized = true;
        return true;
    }

    bool IWithRendering::initDebugDrawing() {
        m_debugDraw = new utils::SimpleDebugDraw();
        if (!m_debugDraw->init(m_shaderCompiler, m_swapChain, m_renderPass, m_vboFactory, m_uboFactory, m_descriptorFactory)) {
            delete m_debugDraw;
            return false;
        }

        m_window->subscribe(m_debugDraw);

        return true;
    }
    
    bool IWithRendering::initImGui() {
        m_imgui = new utils::ImGuiContext(m_renderPass, m_swapChain, m_logicalDevice->getGraphicsQueue());
        if (!m_imgui->init()) {
            delete m_imgui;
            return false;
        }

        m_window->subscribe(m_imgui);

        return true;
    }

    void IWithRendering::shutdownRendering() {
        if (m_imgui) {
            m_window->unsubscribe(m_imgui);

            delete m_imgui;
            m_imgui = nullptr;
        }

        if (m_debugDraw) {
            m_window->unsubscribe(m_debugDraw);
            
            delete m_debugDraw;
            m_debugDraw = nullptr;
        }

        if (m_descriptorFactory) {
            delete m_descriptorFactory;
            m_descriptorFactory = nullptr;
        }

        if (m_uboFactory) {
            delete m_uboFactory;
            m_uboFactory = nullptr;
        }

        if (m_vboFactory) {
            delete m_vboFactory;
            m_vboFactory = nullptr;
        }

        if (m_shaderCompiler) {
            delete m_shaderCompiler;
            m_shaderCompiler = nullptr;
        }

        if (m_frames) {
            delete m_frames;
            m_frames = nullptr;
        }

        if (m_renderPass) {
            delete m_renderPass;
            m_renderPass = nullptr;
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

        if (m_window) {
            m_window->unsubscribe(this);
            m_window = nullptr;
        }

        m_initialized = false;
    }



    const vulkan::PhysicalDevice* IWithRendering::choosePhysicalDevice(const Array<vulkan::PhysicalDevice>& devices) {
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
    
    void IWithRendering::onWindowResize(::utils::Window* win, u32 width, u32 height) {
        if (!m_initialized || win != m_window) return;
        log("Window resized, recreating swapchain (%dx%d)", width, height);
        m_logicalDevice->waitForIdle();
        if (!m_swapChain->recreate()) {
            fatal("Failed to recreate swapchain after window resized");
            shutdownRendering();
        }

        m_frames->shutdown();
        if (!m_frames->init()) {
            fatal("Failed to recreate frame manager after window resized");
            shutdownRendering();
        }
    }

    ::utils::Window* IWithRendering::getWindow() const {
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
    
    vulkan::RenderPass* IWithRendering::getRenderPass() const {
        return m_renderPass;
    }

    vulkan::ShaderCompiler* IWithRendering::getShaderCompiler() const {
        return m_shaderCompiler;
    }
    
    utils::SimpleDebugDraw* IWithRendering::getDebugDraw() const {
        return m_debugDraw;
    }
    
    utils::ImGuiContext* IWithRendering::getImGui() const {
        return m_imgui;
    }
    
    core::FrameManager* IWithRendering::getFrameManager() const {
        return m_frames;
    }

    vulkan::Vertices* IWithRendering::allocateVertices(core::DataFormat* fmt, u32 count) {
        return m_vboFactory->allocate(fmt, count);
    }
    
    vulkan::UniformObject* IWithRendering::allocateUniformObject(core::DataFormat* fmt) {
        return m_uboFactory->allocate(fmt);
    }
    
    vulkan::DescriptorSet* IWithRendering::allocateDescriptor(vulkan::Pipeline* pipeline) {
        return m_descriptorFactory->allocate(pipeline);
    }

    core::FrameContext* IWithRendering::getFrame() {
        if (!m_frames) return nullptr;

        return m_frames->getFrame();
    }

    void IWithRendering::releaseFrame(core::FrameContext* frame) {
        if (!m_frames) return;

        m_frames->releaseFrame(frame);
    }
};