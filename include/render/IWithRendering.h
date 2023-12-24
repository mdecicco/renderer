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
        class RenderPass;
        class VertexBufferFactory;
        class Vertices;
        class UniformBufferFactory;
        class UniformObject;
        class DescriptorFactory;
        class DescriptorSet;
        class Texture;
    };

    namespace core {
        class DataFormat;
        class FrameManager;
        class FrameContext;
    };
    
    namespace utils {
        class SimpleDebugDraw;
        class ImGuiContext;
    }

    class IWithRendering : public ::utils::IWithLogging, ::utils::IInputHandler {
        public:
            IWithRendering();
            virtual ~IWithRendering();

            bool initRendering(::utils::Window* win);
            bool initDebugDrawing(u32 maxLines = 4096);
            bool initImGui();
            void shutdownRendering();

            virtual const vulkan::PhysicalDevice* choosePhysicalDevice(const Array<vulkan::PhysicalDevice>& devices);
            virtual bool setupInstance(vulkan::Instance* instance);
            virtual bool setupDevice(vulkan::LogicalDevice* device);
            virtual bool setupSwapchain(vulkan::SwapChain* swapChain, const vulkan::SwapChainSupport& support);
            virtual bool setupShaderCompiler(vulkan::ShaderCompiler* shaderCompiler);
            virtual void onWindowResize(::utils::Window* win, u32 width, u32 height);

            ::utils::Window* getWindow() const;
            vulkan::Instance* getInstance() const;
            vulkan::PhysicalDevice* getPhysicalDevice() const;
            vulkan::LogicalDevice* getLogicalDevice() const;
            vulkan::Surface* getSurface() const;
            vulkan::SwapChain* getSwapChain() const;
            vulkan::RenderPass* getRenderPass() const;
            vulkan::ShaderCompiler* getShaderCompiler() const;
            utils::SimpleDebugDraw* getDebugDraw() const;
            utils::ImGuiContext* getImGui() const;
            core::FrameManager* getFrameManager() const;

            vulkan::Vertices* allocateVertices(core::DataFormat* format, u32 count);
            vulkan::UniformObject* allocateUniformObject(core::DataFormat* format);
            vulkan::DescriptorSet* allocateDescriptor(vulkan::Pipeline* pipeline);
            core::FrameContext* getFrame();
            void releaseFrame(core::FrameContext* frame);
        
        private:
            ::utils::Window* m_window;
            vulkan::Instance* m_instance;
            vulkan::PhysicalDevice* m_physicalDevice;
            vulkan::LogicalDevice* m_logicalDevice;
            vulkan::Surface* m_surface;
            vulkan::SwapChain* m_swapChain;
            vulkan::RenderPass* m_renderPass;
            vulkan::ShaderCompiler* m_shaderCompiler;
            vulkan::VertexBufferFactory* m_vboFactory;
            vulkan::UniformBufferFactory* m_uboFactory;
            vulkan::DescriptorFactory* m_descriptorFactory;
            utils::SimpleDebugDraw* m_debugDraw;
            utils::ImGuiContext* m_imgui;
            core::FrameManager* m_frames;

            bool m_initialized;
    };
};