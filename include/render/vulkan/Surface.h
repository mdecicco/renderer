#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>

namespace utils {
    class Window;
};

namespace render {
    namespace vulkan {
        class Instance;
        class Surface {
            public:
                Surface(Instance* instance, ::utils::Window* window);
                ~Surface();

                VkSurfaceKHR get() const;
                ::utils::Window* getWindow() const;
                bool isInitialized() const;

                bool init();
                void shutdown();
            
            protected:
                Instance* m_instance;
                ::utils::Window* m_window;
                VkSurfaceKHR m_surface;
        };
    };
};