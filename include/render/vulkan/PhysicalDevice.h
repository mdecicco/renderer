#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class Instance;
        class Surface;
        class QueueFamily;
        class SwapChainSupport;

        class PhysicalDevice {
            public:
                PhysicalDevice(const PhysicalDevice& dev);
                ~PhysicalDevice();
                static Array<PhysicalDevice> list(Instance* instance);

                bool isDiscrete() const;
                bool isVirtual() const;
                bool isIntegrated() const;
                bool isCPU() const;
                bool isExtensionAvailable(const char* name) const;
                bool isLayerAvailable(const char* name) const;
                
                bool canPresentToSurface(Surface* surface, const QueueFamily& queueFamily) const;
                bool getSurfaceSwapChainSupport(Surface* surface, SwapChainSupport* out) const;
                bool getMemoryTypeIndex(const VkMemoryRequirements& reqs, VkMemoryPropertyFlags flags, u32* dst) const;

                VkPhysicalDevice get() const;
                const VkPhysicalDeviceProperties& getProperties() const;
                const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const;
                const VkPhysicalDeviceFeatures& getFeatures() const;
                Instance* getInstance() const;

            protected:
                friend class ::utils::Array<PhysicalDevice>;
                PhysicalDevice();

                Instance* m_instance;
                VkPhysicalDevice m_handle;
                VkPhysicalDeviceProperties m_props;
                VkPhysicalDeviceFeatures m_features;
                VkPhysicalDeviceMemoryProperties m_memoryProps;
                Array<VkExtensionProperties> m_availableExtensions;
                Array<VkLayerProperties> m_availableLayers;
        };
    };
};