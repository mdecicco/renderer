#include <exception>
#include <stdio.h>

#include <utils/Allocator.hpp>
#include <utils/Singleton.hpp>
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

class logger : public utils::ILogListener {
    public:
        virtual void onLogMessage(utils::LOG_LEVEL level, const utils::String& scope, const utils::String& message) {
            utils::String msg = scope + ": " + message;
            printf("%s\n", msg.c_str());
        }
};

int main(int argc, char** argv) {
    utils::Mem::Create();

    {
        utils::Window w;
        w.setPosition(500, 500);
        w.setSize(800, 600);
        w.setTitle("Vulkan Test");
        if (!w.setOpen(true)) abort();

        render::vulkan::Instance vi;
        logger l;

        vi.subscribeLogger(&l);
        vi.enableValidation();
        vi.initialize();

        render::vulkan::Surface s(&vi, &w);
        if (!s.init()) abort();

        auto devices = render::vulkan::PhysicalDevice::list(&vi);

        render::vulkan::PhysicalDevice* gpu = nullptr;
        render::vulkan::SwapChainSupport swapChainSupport;

        for (render::u32 i = 0;i < devices.size() && !gpu;i++) {
            if (!devices[i].isDiscrete()) continue;
            if (!devices[i].isExtensionAvailable(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;

            if (!devices[i].getSurfaceSwapChainSupport(&s, &swapChainSupport)) continue;
            if (!swapChainSupport.isValid()) continue;

            if (!swapChainSupport.hasFormat(VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) continue;
            if (!swapChainSupport.hasPresentMode(VK_PRESENT_MODE_FIFO_KHR)) continue;

            auto& capabilities = swapChainSupport.getCapabilities();
            if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < 3) continue;

            gpu = &devices[i];
        }

        if (!gpu) abort();

        render::vulkan::LogicalDevice dev(gpu);
        if (!dev.enableExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) abort();
        if (!dev.init(true, false, false, &s)) abort();

        render::vulkan::SwapChain chain;
        chain.init(
            &s,
            &dev,
            swapChainSupport,
            VK_FORMAT_B8G8R8A8_SRGB,
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            VK_PRESENT_MODE_FIFO_KHR,
            3
        );

        if (!chain.isValid()) abort();

        render::vulkan::ShaderCompiler sh;
        sh.subscribeLogger(&l);
        if (!sh.init()) abort();

        auto out = sh.compileShader(
            "layout (location = 1) in vec4 v_pos;\n"
            "layout (set = 0, binding = 0) uniform Matrices {\n"
            "    uniform mat4 model;\n"
            "    uniform mat4 view;\n"
            "    uniform mat4 projection;\n"
            "} matrices;\n"
            "\n"
            "void main() {\n"
            "  gl_Position = matrices.projection * matrices.view * matrices.model * v_pos;\n"
            "}\n",
            glslang::EShLanguage::
        );

        if (!out) abort();

        delete out;

        utils::Thread::Sleep(1000);
    }

    utils::Mem::Destroy();
    return 0;
}