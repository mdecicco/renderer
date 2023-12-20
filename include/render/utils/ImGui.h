#pragma once
#include <render/types.h>

#include <utils/Timer.h>
#include <utils/Input.h>
#include <vulkan/vulkan.h>
#include <imgui.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        class Queue;
        class RenderPass;
        class SwapChain;
    };

    namespace core {
        class FrameContext;
    };

    namespace utils {
        class ImGuiContext : public ::utils::IInputHandler {
            public:
                ImGuiContext(
                    vulkan::RenderPass* renderPass,
                    vulkan::SwapChain* swapChain,
                    const vulkan::Queue* graphicsQueue
                );
                ~ImGuiContext();

                bool init();
                void shutdown();

                void begin();
                void end(core::FrameContext* frame);

                virtual void onMouseMove(i32 x, i32 y);
                virtual void onScroll(f32 delta);
                virtual void onMouseDown(::utils::MouseButton buttonIdx);
                virtual void onMouseUp(::utils::MouseButton buttonIdx);
                virtual void onChar(u8 code);
                virtual void onKeyDown(::utils::KeyboardKey key);
                virtual void onKeyUp(::utils::KeyboardKey key);
            
            private:
                bool m_contextCreated;
                bool m_libInitialized;
                bool m_fontsInitialized;
                vulkan::LogicalDevice* m_device;
                const vulkan::Queue* m_gfxQueue;
                vulkan::RenderPass* m_renderPass;
                vulkan::SwapChain* m_swapChain;
                vec2f m_wheel;
                ::utils::Timer m_dt;

                VkDescriptorPool m_descriptorPool;
        };
    };
};