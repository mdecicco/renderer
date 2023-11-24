#pragma once
#include <render/types.h>

#include <utils/Array.h>
#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class Instance;
        class PhysicalDevice;
        class Queue;
        class QueueFamily;
        class Surface;

        class LogicalDevice {
            public:
                LogicalDevice(PhysicalDevice* device);
                ~LogicalDevice();

                bool isInitialized() const;
                bool enableExtension(const char* name);
                bool isExtensionEnabled(const char* name) const;
                bool enableLayer(const char* name);
                bool isLayerEnabled(const char* name) const;

                bool init(bool needsGraphics, bool needsCompute, bool needsTransfer, Surface* surface);
                void shutdown();

                VkDevice get() const;
                PhysicalDevice* getPhysicalDevice() const;
                Instance* getInstance() const;
                const utils::Array<Queue*>& getQueues() const;
                const Queue* getPresentationQueue() const;
                const Queue* getGraphicsQueue() const;
            
            protected:
                u32 buildQueueInfo(
                    utils::Array<QueueFamily>& families,
                    VkDeviceQueueCreateInfo* infos,
                    bool needsGraphics,
                    bool needsCompute,
                    bool needsTransfer,
                    Surface* surface,
                    i32* outSurfaceQueueInfoIdx,
                    i32* outGfxQueueInfoIdx
                ) const;

                bool m_isInitialized;
                VkDevice m_device;
                PhysicalDevice* m_physicalDevice;
                utils::Array<const char*> m_enabledExtensions;
                utils::Array<const char*> m_enabledLayers;
                utils::Array<Queue*> m_queues;
                Queue* m_presentQueue;
                Queue* m_gfxQueue;
        };
    };
};