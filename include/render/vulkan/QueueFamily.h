#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class PhysicalDevice;
        class Instance;

        class QueueFamily {
            public:
                QueueFamily(const QueueFamily& qf);
                ~QueueFamily();

                bool supportsGraphics() const;
                bool supportsCompute() const;
                bool supportsTransfer() const;

                const VkQueueFamilyProperties& getProperties() const;
                PhysicalDevice* getDevice() const;
                Instance* getInstance() const;
                i32 getIndex() const;

                static Array<QueueFamily> list(PhysicalDevice* device);
            
            protected:
                friend class ::utils::Array<QueueFamily>;
                QueueFamily();

                VkQueueFamilyProperties m_props;
                PhysicalDevice* m_device;
                i32 m_index;
        };
    };
};