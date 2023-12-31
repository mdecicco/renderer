#pragma once
#include <render/types.h>

#include <utils/String.h>
#include <utils/Array.h>
#include <utils/ILogListener.h>
#include <vulkan/vulkan.h>

namespace utils {
    class GeneralAllocator;
};

namespace render {
    namespace vulkan {
        class Instance : public ::utils::IWithLogging {
            public:
                Instance();
                ~Instance();

                void enableValidation();
                void setApplicationName(const String& name);
                void setApplicationVersion(u32 major, u32 minor, u32 patch);
                void setEngineName(const String& name);
                void setEngineVersion(u32 major, u32 minor, u32 patch);
                bool enableExtension(const char* name);
                bool enableLayer(const char* name);
                bool isExtensionAvailable(const char* name) const;
                bool isLayerAvailable(const char* name) const;
                bool isExtensionEnabled(const char* name) const;
                bool isLayerEnabled(const char* name) const;
                bool isValidationEnabled() const;

                bool isInitialized() const;
                bool initialize();
                void shutdown(bool doResetConfiguration);

                bool onLogMessage(
                    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
                );

                VkInstance get();
                const VkAllocationCallbacks* getAllocator() const;

            protected:
                bool m_isInitialized;
                VkInstance m_instance;
                VkAllocationCallbacks m_allocatorCallbacks;
                VkDebugUtilsMessengerEXT m_logger;

                bool m_validationEnabled;
                bool m_canInterceptLogs;
                String m_applicationName;
                String m_engineName;
                u32 m_applicationVersion;
                u32 m_engineVersion;
                Array<VkExtensionProperties> m_availableExtensions;
                Array<VkLayerProperties> m_availableLayers;
                Array<const char*> m_enabledExtensions;
                Array<const char*> m_enabledLayers;
        };
    };
};