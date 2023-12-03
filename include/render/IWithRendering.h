#pragma once
#include <render/types.h>

#include <utils/ILogListener.h>
#include <utils/Array.h>
#include <utils/Input.h>

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
        class VertexBufferFactory;
        class Vertices;
        class UniformBufferFactory;
        class UniformObject;
    };

    namespace core {
        class FrameManager;
        class FrameContext;
        class DataFormat;
    };

    class IWithRendering : public utils::IWithLogging, utils::IInputHandler {
        public:
            IWithRendering();
            virtual ~IWithRendering();

            bool initRendering(utils::Window* win);
            void shutdownRendering();

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
            vulkan::Vertices* allocateVertices(core::DataFormat* format, u32 count);
            vulkan::UniformObject* allocateUniformObject(core::DataFormat* format, u32 bindIndex, vulkan::Pipeline* pipeline);
        
        private:
            utils::Window* m_window;
            vulkan::Instance* m_instance;
            vulkan::PhysicalDevice* m_physicalDevice;
            vulkan::LogicalDevice* m_logicalDevice;
            vulkan::Surface* m_surface;
            vulkan::SwapChain* m_swapChain;
            vulkan::ShaderCompiler* m_shaderCompiler;
            vulkan::VertexBufferFactory* m_vboFactory;
            vulkan::UniformBufferFactory* m_uboFactory;
            core::FrameManager* m_frameMgr;

            bool m_initialized;
    };
};