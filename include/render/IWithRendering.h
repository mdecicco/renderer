#pragma once
#include <render/types.h>

#include <utils/ILogListener.h>
#include <utils/Array.h>
#include <utils/Input.h>
#include <vulkan/vulkan.h>

namespace utils {
    class Window;
};

namespace render {
    namespace vulkan {
        class Instance;
        class PhysicalDevice;
        class Surface;
        class SwapChain;
        class SwapChainSupport;
        class ShaderCompiler;
        class LogicalDevice;
        class CommandBuffer;
        class Pipeline;
    };

    namespace core {
        class FrameManager;
        class FrameContext;
    };

    class IWithRendering : public utils::IWithLogging, utils::IInputHandler {
        public:
            IWithRendering();
            virtual ~IWithRendering();

            virtual const vulkan::PhysicalDevice* choosePhysicalDevice(const utils::Array<vulkan::PhysicalDevice>& devices);
            virtual bool setupInstance(vulkan::Instance* instance);
            virtual bool setupDevice(vulkan::LogicalDevice* device);
            virtual bool setupSwapchain(vulkan::SwapChain* swapChain, const vulkan::SwapChainSupport& support);
            virtual bool setupShaderCompiler(vulkan::ShaderCompiler* shaderCompiler);
            virtual void onWindowResize(utils::Window* win, u32 width, u32 height);

            utils::Window* getWindow() const;
            vulkan::Instance* getInstance() const;
            vulkan::PhysicalDevice* getPhysicalDevice() const;
            vulkan::LogicalDevice* getLogicalDevice() const;
            vulkan::Surface* getSurface() const;
            vulkan::SwapChain* getSwapChain() const;
            vulkan::ShaderCompiler* getShaderCompiler() const;
            core::FrameManager* getFrameManager() const;
            core::FrameContext* getFrame(vulkan::Pipeline* pipeline) const;
            void releaseFrame(core::FrameContext* frame);

            bool initRendering(utils::Window* win);
            void shutdownRendering();
        
        private:
            utils::Window* m_window;
            vulkan::Instance* m_instance;
            vulkan::PhysicalDevice* m_physicalDevice;
            vulkan::LogicalDevice* m_logicalDevice;
            vulkan::Surface* m_surface;
            vulkan::SwapChain* m_swapChain;
            vulkan::ShaderCompiler* m_shaderCompiler;
            core::FrameManager* m_frameMgr;

            bool m_initialized;
    };
};