#pragma once
#include <render/types.h>

#include <utils/ILogListener.h>
#include <utils/Array.h>
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


    class IWithRendering : public utils::IWithLogging {
        public:
            IWithRendering();
            virtual ~IWithRendering();

            virtual const vulkan::PhysicalDevice* choosePhysicalDevice(const utils::Array<vulkan::PhysicalDevice>& devices);
            virtual bool setupInstance(vulkan::Instance* instance);
            virtual bool setupDevice(vulkan::LogicalDevice* device);
            virtual bool setupSwapchain(vulkan::SwapChain* swapChain, const vulkan::SwapChainSupport& support);
            virtual bool setupShaderCompiler(vulkan::ShaderCompiler* shaderCompiler);

            bool begin(vulkan::CommandBuffer* cb, vulkan::Pipeline* pipeline);
            bool end(vulkan::CommandBuffer* cb, vulkan::Pipeline* pipeline);
            bool waitForFrameEnd();

            utils::Window* getWindow() const;
            vulkan::Instance* getInstance() const;
            vulkan::PhysicalDevice* getPhysicalDevice() const;
            vulkan::LogicalDevice* getLogicalDevice() const;
            vulkan::Surface* getSurface() const;
            vulkan::SwapChain* getSwapChain() const;
            vulkan::ShaderCompiler* getShaderCompiler() const;

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
            VkSemaphore m_swapChainReady;
            VkSemaphore m_renderComplete;
            VkFence m_frameFence;

            bool m_initialized;
            bool m_frameStarted;
            u32 m_currentImageIndex;
    };
};