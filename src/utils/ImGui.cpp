#include <render/utils/ImGui.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/Queue.h>
#include <render/vulkan/QueueFamily.h>
#include <render/vulkan/RenderPass.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/core/FrameContext.h>

#include <backends/imgui_impl_vulkan.h>

ImGuiKey getImGuiKeyCode(utils::KeyboardKey key) {
    switch (key) {
        case utils::KEY_TAB: return ImGuiKey_Tab;
        case utils::KEY_LEFT: return ImGuiKey_LeftArrow;
        case utils::KEY_RIGHT: return ImGuiKey_RightArrow;
        case utils::KEY_UP: return ImGuiKey_UpArrow;
        case utils::KEY_DOWN: return ImGuiKey_DownArrow;
        case utils::KEY_PAGE_UP: return ImGuiKey_PageUp;
        case utils::KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case utils::KEY_HOME: return ImGuiKey_Home;
        case utils::KEY_END: return ImGuiKey_End;
        case utils::KEY_INSERT: return ImGuiKey_Insert;
        case utils::KEY_DELETE: return ImGuiKey_Delete;
        case utils::KEY_BACKSPACE: return ImGuiKey_Backspace;
        case utils::KEY_SPACE: return ImGuiKey_Space;
        case utils::KEY_ENTER: return ImGuiKey_Enter;
        case utils::KEY_ESCAPE: return ImGuiKey_Escape;
        case utils::KEY_SINGLE_QUOTE: return ImGuiKey_Apostrophe;
        case utils::KEY_COMMA: return ImGuiKey_Comma;
        case utils::KEY_MINUS: return ImGuiKey_Minus;
        case utils::KEY_PERIOD: return ImGuiKey_Period;
        case utils::KEY_SLASH: return ImGuiKey_Slash;
        case utils::KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case utils::KEY_EQUAL: return ImGuiKey_Equal;
        case utils::KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
        case utils::KEY_BACKSLASH: return ImGuiKey_Backslash;
        case utils::KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
        case utils::KEY_BACKTICK: return ImGuiKey_GraveAccent;
        case utils::KEY_CAP_LOCK: return ImGuiKey_CapsLock;
        case utils::KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
        case utils::KEY_NUM_LOCK: return ImGuiKey_NumLock;
        case utils::KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
        case utils::KEY_PAUSE: return ImGuiKey_Pause;
        case utils::KEY_NUMPAD_0: return ImGuiKey_Keypad0;
        case utils::KEY_NUMPAD_1: return ImGuiKey_Keypad1;
        case utils::KEY_NUMPAD_2: return ImGuiKey_Keypad2;
        case utils::KEY_NUMPAD_3: return ImGuiKey_Keypad3;
        case utils::KEY_NUMPAD_4: return ImGuiKey_Keypad4;
        case utils::KEY_NUMPAD_5: return ImGuiKey_Keypad5;
        case utils::KEY_NUMPAD_6: return ImGuiKey_Keypad6;
        case utils::KEY_NUMPAD_7: return ImGuiKey_Keypad7;
        case utils::KEY_NUMPAD_8: return ImGuiKey_Keypad8;
        case utils::KEY_NUMPAD_9: return ImGuiKey_Keypad9;
        case utils::KEY_NUMPAD_DECIMAL: return ImGuiKey_KeypadDecimal;
        case utils::KEY_NUMPAD_DIVIDE: return ImGuiKey_KeypadDivide;
        case utils::KEY_NUMPAD_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case utils::KEY_NUMPAD_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case utils::KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case utils::KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        case utils::KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        case utils::KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case utils::KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case utils::KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        case utils::KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        case utils::KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case utils::KEY_0: return ImGuiKey_0;
        case utils::KEY_1: return ImGuiKey_1;
        case utils::KEY_2: return ImGuiKey_2;
        case utils::KEY_3: return ImGuiKey_3;
        case utils::KEY_4: return ImGuiKey_4;
        case utils::KEY_5: return ImGuiKey_5;
        case utils::KEY_6: return ImGuiKey_6;
        case utils::KEY_7: return ImGuiKey_7;
        case utils::KEY_8: return ImGuiKey_8;
        case utils::KEY_9: return ImGuiKey_9;
        case utils::KEY_A: return ImGuiKey_A;
        case utils::KEY_B: return ImGuiKey_B;
        case utils::KEY_C: return ImGuiKey_C;
        case utils::KEY_D: return ImGuiKey_D;
        case utils::KEY_E: return ImGuiKey_E;
        case utils::KEY_F: return ImGuiKey_F;
        case utils::KEY_G: return ImGuiKey_G;
        case utils::KEY_H: return ImGuiKey_H;
        case utils::KEY_I: return ImGuiKey_I;
        case utils::KEY_J: return ImGuiKey_J;
        case utils::KEY_K: return ImGuiKey_K;
        case utils::KEY_L: return ImGuiKey_L;
        case utils::KEY_M: return ImGuiKey_M;
        case utils::KEY_N: return ImGuiKey_N;
        case utils::KEY_O: return ImGuiKey_O;
        case utils::KEY_P: return ImGuiKey_P;
        case utils::KEY_Q: return ImGuiKey_Q;
        case utils::KEY_R: return ImGuiKey_R;
        case utils::KEY_S: return ImGuiKey_S;
        case utils::KEY_T: return ImGuiKey_T;
        case utils::KEY_U: return ImGuiKey_U;
        case utils::KEY_V: return ImGuiKey_V;
        case utils::KEY_W: return ImGuiKey_W;
        case utils::KEY_X: return ImGuiKey_X;
        case utils::KEY_Y: return ImGuiKey_Y;
        case utils::KEY_Z: return ImGuiKey_Z;
        case utils::KEY_F1: return ImGuiKey_F1;
        case utils::KEY_F2: return ImGuiKey_F2;
        case utils::KEY_F3: return ImGuiKey_F3;
        case utils::KEY_F4: return ImGuiKey_F4;
        case utils::KEY_F5: return ImGuiKey_F5;
        case utils::KEY_F6: return ImGuiKey_F6;
        case utils::KEY_F7: return ImGuiKey_F7;
        case utils::KEY_F8: return ImGuiKey_F8;
        case utils::KEY_F9: return ImGuiKey_F9;
        case utils::KEY_F10: return ImGuiKey_F10;
        case utils::KEY_F11: return ImGuiKey_F11;
        case utils::KEY_F12: return ImGuiKey_F12;
        case utils::KEY_F13: return ImGuiKey_F13;
        case utils::KEY_F14: return ImGuiKey_F14;
        case utils::KEY_F15: return ImGuiKey_F15;
        case utils::KEY_F16: return ImGuiKey_F16;
        case utils::KEY_F17: return ImGuiKey_F17;
        case utils::KEY_F18: return ImGuiKey_F18;
        case utils::KEY_F19: return ImGuiKey_F19;
        case utils::KEY_F20: return ImGuiKey_F20;
        case utils::KEY_F21: return ImGuiKey_F21;
        case utils::KEY_F22: return ImGuiKey_F22;
        case utils::KEY_F23: return ImGuiKey_F23;
        case utils::KEY_F24: return ImGuiKey_F24;
        default: return ImGuiKey_None;
    }
}

namespace render {
    namespace utils {
        ImGuiContext::ImGuiContext(
            vulkan::RenderPass* renderPass,
            const vulkan::Queue* graphicsQueue
        ) {
            m_device = renderPass->getSwapChain()->getDevice();
            m_gfxQueue = graphicsQueue;
            m_renderPass = renderPass;
            m_descriptorPool = VK_NULL_HANDLE;
            m_contextCreated = false;
            m_libInitialized = false;
            m_fontsInitialized = false;
        }

        ImGuiContext::~ImGuiContext() {
            shutdown();
        }

        bool ImGuiContext::init() {
            VkDescriptorPoolSize pool_sizes[] = {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

            VkDescriptorPoolCreateInfo pi = {};
            pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pi.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pi.maxSets = 1000;
            pi.poolSizeCount = std::size(pool_sizes);
            pi.pPoolSizes = pool_sizes;

            if (vkCreateDescriptorPool(m_device->get(), &pi, m_device->getInstance()->getAllocator(), &m_descriptorPool) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to create descriptor pool for ImGui");
                shutdown();
                return false;
            }

            if (!ImGui::CreateContext()) {
                m_device->getInstance()->error("Failed to create ImGui context");
                shutdown();
                return false;
            }
            m_contextCreated = true;

            //this initializes imgui for Vulkan
            ImGui_ImplVulkan_InitInfo ii = {};
            ii.Instance = m_device->getInstance()->get();
            ii.Allocator = m_device->getInstance()->getAllocator();
            ii.PhysicalDevice = m_device->getPhysicalDevice()->get();
            ii.Device = m_device->get();
            ii.Queue = m_gfxQueue->get();
            ii.QueueFamily = m_gfxQueue->getFamily().getIndex();
            ii.DescriptorPool = m_descriptorPool;
            ii.ImageCount = m_renderPass->getSwapChain()->getImageViews().size();
            ii.MinImageCount = ii.ImageCount;
            ii.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

            if (!ImGui_ImplVulkan_Init(&ii, m_renderPass->get())) {
                m_device->getInstance()->error("Failed to initialize ImGui library");
                shutdown();
                return false;
            }
            m_libInitialized = true;

            
            if (!ImGui_ImplVulkan_CreateFontsTexture()) {
                m_device->getInstance()->error("Failed to initialize ImGui fonts");
                shutdown();
                return false;
            }
            m_fontsInitialized = true;

            ImGuiIO& io = ImGui::GetIO();
            io.BackendPlatformName = "render_utils";
            io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

            return true;
        }

        void ImGuiContext::shutdown() {
            if (m_fontsInitialized) {
                ImGui_ImplVulkan_DestroyFontsTexture();
                m_fontsInitialized = false;
            }

            if (m_libInitialized) {
                ImGui_ImplVulkan_Shutdown();
                m_libInitialized = false;
            }

            if (m_contextCreated) {
                ImGui::DestroyContext();
                m_contextCreated = false;
            }

            if (m_descriptorPool) {
                vkDestroyDescriptorPool(m_device->get(), m_descriptorPool, m_device->getInstance()->getAllocator());
                m_descriptorPool = VK_NULL_HANDLE;
            }
        }

        void ImGuiContext::begin() {
            f32 dt = m_dt;
            m_dt.reset();
            m_dt.start();

            ImGui_ImplVulkan_NewFrame();

            vulkan::SwapChain* sc = m_renderPass->getSwapChain();
            auto& dims = sc->getExtent();

            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(dims.width, dims.height);
            io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f); // todo
            io.DeltaTime = dt > 0.0f ? dt : (1.0f / 60.0f);

            ImGui::NewFrame();
        }

        void ImGuiContext::end(core::FrameContext* frame) {
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(
                ImGui::GetDrawData(),
                frame->getCommandBuffer()->get()
            );
        }

        void ImGuiContext::onMouseMove(i32 x, i32 y) {
            ImGuiIO& io = ImGui::GetIO();
            io.AddMousePosEvent(x, y);
        }

        void ImGuiContext::onScroll(f32 delta) {
            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseWheelEvent(0.0f, delta);
        }

        void ImGuiContext::onMouseDown(::utils::MouseButton buttonIdx) {
            ImGuiMouseButton btn;
            switch (buttonIdx) {
                case ::utils::MouseButton::LEFT_BUTTON: { btn = ImGuiMouseButton_Left; break; }
                case ::utils::MouseButton::MIDDLE_BUTTON: { btn = ImGuiMouseButton_Middle; break; }
                case ::utils::MouseButton::RIGHT_BUTTON: { btn = ImGuiMouseButton_Right; break; }
            }

            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseButtonEvent(btn, true);
        }

        void ImGuiContext::onMouseUp(::utils::MouseButton buttonIdx) {
            ImGuiMouseButton btn;
            switch (buttonIdx) {
                case ::utils::MouseButton::LEFT_BUTTON: { btn = ImGuiMouseButton_Left; break; }
                case ::utils::MouseButton::MIDDLE_BUTTON: { btn = ImGuiMouseButton_Middle; break; }
                case ::utils::MouseButton::RIGHT_BUTTON: { btn = ImGuiMouseButton_Right; break; }
            }

            ImGuiIO& io = ImGui::GetIO();
            io.AddMouseButtonEvent(btn, false);
        }
        
        void ImGuiContext::onChar(u8 code) {
            ImGuiIO& io = ImGui::GetIO();
            io.AddInputCharacter(code);
        }
        
        void ImGuiContext::onKeyDown(::utils::KeyboardKey key) {
            ImGuiIO& io = ImGui::GetIO();
            io.AddKeyEvent(getImGuiKeyCode(key), true);
        }

        void ImGuiContext::onKeyUp(::utils::KeyboardKey key) {
            ImGuiIO& io = ImGui::GetIO();
            io.AddKeyEvent(getImGuiKeyCode(key), false);
        }
    };
};