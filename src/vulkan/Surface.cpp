#include <render/vulkan/Surface.h>
#include <render/vulkan/Instance.h>

#include <utils/Window.h>

#ifdef _WIN32
    #include <Windows.h>
    #include <vulkan/vulkan_win32.h>
#endif

namespace render {
    namespace vulkan {
        Surface::Surface(Instance* instance, utils::Window* window) {
            m_surface = nullptr;
            m_instance = instance;
            m_window = window;
        }

        Surface::~Surface() {
            shutdown();
        }

        VkSurfaceKHR Surface::get() const {
            return m_surface;
        }

        utils::Window* Surface::getWindow() const {
            return m_window;
        }

        bool Surface::isInitialized() const {
            return m_surface != nullptr;
        }

        bool Surface::init() {
            if (!m_instance || !m_window || m_surface) return false;

            #ifdef _WIN32
                VkWin32SurfaceCreateInfoKHR ci = {};
                ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                ci.hwnd = m_window->getHandle();
                ci.hinstance = GetModuleHandle(nullptr);

                if (vkCreateWin32SurfaceKHR(m_instance->get(), &ci, m_instance->getAllocator(), &m_surface) != VK_SUCCESS) {
                    return false;
                }

                return true;
            #endif

            return false;
        }

        void Surface::shutdown() {
            if (!m_surface) return;
            vkDestroySurfaceKHR(m_instance->get(), m_surface, m_instance->getAllocator());
        }
    };
};