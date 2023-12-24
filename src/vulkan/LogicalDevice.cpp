#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/QueueFamily.h>
#include <render/vulkan/Queue.h>

#include <utils/Array.hpp>

namespace render {
    namespace vulkan {
        LogicalDevice::LogicalDevice(PhysicalDevice* device) {
            m_device = VK_NULL_HANDLE;
            m_physicalDevice = device;
            m_isInitialized = false;
            m_presentQueue = nullptr;
            m_computeQueue = nullptr;
            m_gfxQueue = nullptr;
        }

        LogicalDevice::~LogicalDevice() {
            shutdown();
        }

        bool LogicalDevice::isInitialized() const {
            return m_isInitialized;
        }

        bool LogicalDevice::enableExtension(const char* name) {
            if (m_isInitialized || !m_physicalDevice) return false;

            if (isExtensionEnabled(name)) return true;
            if (!m_physicalDevice->isExtensionAvailable(name)) return false;

            m_enabledExtensions.push(name);
            return true;
        }

        bool LogicalDevice::isExtensionEnabled(const char* name) const {
            for (u32 i = 0;i < m_enabledExtensions.size();i++) {
                if (strcmp(name, m_enabledExtensions[i]) == 0) {
                    return true;
                }
            }

            return false;
        }

        bool LogicalDevice::enableLayer(const char* name) {
            if (m_isInitialized || !m_physicalDevice) return false;

            if (isLayerEnabled(name)) return true;
            if (!m_physicalDevice->isLayerAvailable(name)) return false;

            m_enabledLayers.push(name);
            return true;
        }

        bool LogicalDevice::isLayerEnabled(const char* name) const {
            for (u32 i = 0;i < m_enabledLayers.size();i++) {
                if (strcmp(name, m_enabledLayers[i]) == 0) {
                    return true;
                }
            }

            return false;
        }

        bool LogicalDevice::waitForIdle() const {
            if (!m_isInitialized) return true;
            return vkDeviceWaitIdle(m_device) == VK_SUCCESS;
        }

        bool LogicalDevice::init(bool needsGraphics, bool needsCompute, bool needsTransfer, Surface* surface) {
            if (m_isInitialized || !m_physicalDevice) return false;

            auto families = QueueFamily::list(m_physicalDevice);

            VkDeviceQueueCreateInfo queueInfo[4] = { {}, {}, {}, {} };
            i32 surfaceInfoIdx = -1;
            i32 gfxInfoIdx = -1;
            i32 cmpInfoIdx = -1;
            u32 queueCount = buildQueueInfo(
                families,
                queueInfo,
                needsGraphics, needsCompute, needsTransfer,
                surface,
                &surfaceInfoIdx, &cmpInfoIdx, &gfxInfoIdx
            );
            if (queueCount == 0) return false;

            VkPhysicalDeviceFeatures df = {};
            df.samplerAnisotropy = VK_TRUE;

            VkDeviceCreateInfo di = {};
            di.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            di.pQueueCreateInfos = queueInfo;
            di.queueCreateInfoCount = queueCount;
            di.pEnabledFeatures = &df;
            di.enabledExtensionCount = m_enabledExtensions.size();
            di.ppEnabledExtensionNames = m_enabledExtensions.data();
            di.enabledLayerCount = m_enabledLayers.size();
            di.ppEnabledLayerNames = m_enabledLayers.data();

            if (vkCreateDevice(m_physicalDevice->get(), &di, getInstance()->getAllocator(), &m_device) != VK_SUCCESS) {
                return false;
            }

            for (u32 i = 0;i < queueCount;i++) {
                QueueFamily& fam = families[queueInfo[i].queueFamilyIndex];
                Queue* first = nullptr;

                for (u32 q = 0;q < queueInfo[i].queueCount;q++) {
                    Queue* n = new Queue(this, fam, q);
                    if (!first) first = n;
                    m_queues.push(n);
                }

                if (surface && i == surfaceInfoIdx) {
                    m_presentQueue = first;
                }

                if (needsGraphics && i == gfxInfoIdx) {
                    m_gfxQueue = first;
                }

                if (needsCompute && i == cmpInfoIdx) {
                    m_computeQueue = first;
                }
            }

            m_isInitialized = true;
            return true;
        }

        void LogicalDevice::shutdown() {
            if (!m_isInitialized) return;

            m_queues.each([](Queue* q) { delete q; });
            m_queues.clear();
            m_presentQueue = nullptr;
            m_gfxQueue = nullptr;

            vkDestroyDevice(m_device, getInstance()->getAllocator());
            m_isInitialized = false;
        }

        VkDevice LogicalDevice::get() const {
            if (!m_physicalDevice) return nullptr;
            return m_device;
        }

        PhysicalDevice* LogicalDevice::getPhysicalDevice() const {
            return m_physicalDevice;
        }

        Instance* LogicalDevice::getInstance() const {
            if (!m_physicalDevice) return nullptr;
            return m_physicalDevice->getInstance();
        }
        
        const Array<Queue*>& LogicalDevice::getQueues() const {
            return m_queues;
        }
        
        const Queue* LogicalDevice::getPresentationQueue() const {
            return m_presentQueue;
        }
        
        const Queue* LogicalDevice::getComputeQueue() const {
            return m_computeQueue;
        }

        const Queue* LogicalDevice::getGraphicsQueue() const {
            return m_gfxQueue;
        }

        u32 LogicalDevice::buildQueueInfo(
            Array<QueueFamily>& families,
            VkDeviceQueueCreateInfo* infos,
            bool needsGraphics,
            bool needsCompute,
            bool needsTransfer,
            Surface* surface,
            i32* outSurfaceQueueInfoIdx,
            i32* outCmpQueueInfoIdx,
            i32* outGfxQueueInfoIdx
        ) const {
            if (families.size() == 0) return false;

            i32 gfxIdx = -1;
            i32 cmpIdx = -1;
            i32 xfrIdx = -1;
            i32 srfIdx = -1;
            bool gfxSatisfied = !needsGraphics;
            bool cmpSatisfied = !needsCompute;
            bool xfrSatisfied = !needsTransfer;
            bool srfSatisfied = !surface;

            // I'm assuming that it's better to minimize the number of families used

            for (u32 i = 0;i < families.size();i++) {
                if (needsGraphics && gfxIdx == -1 && families[i].supportsGraphics()) {
                    gfxIdx = i;
                    if (needsCompute && cmpIdx != i && families[i].supportsCompute()) cmpIdx = i;
                    if (needsTransfer && xfrIdx != i && families[i].supportsTransfer()) xfrIdx = i;
                    if (surface && srfIdx != i && m_physicalDevice->canPresentToSurface(surface, families[i])) srfIdx = i;
                }
                
                if (needsCompute && cmpIdx == -1 && families[i].supportsCompute()) {
                    cmpIdx = i;
                    if (needsGraphics && gfxIdx != i && families[i].supportsGraphics()) gfxIdx = i;
                    if (needsTransfer && xfrIdx != i && families[i].supportsTransfer()) xfrIdx = i;
                    if (surface && srfIdx != i && srfIdx != gfxIdx && m_physicalDevice->canPresentToSurface(surface, families[i])) srfIdx = i;
                }
                
                if (needsTransfer && xfrIdx == -1 && families[i].supportsTransfer()) {
                    xfrIdx = i;
                    if (needsGraphics && gfxIdx != i && families[i].supportsGraphics()) gfxIdx = i;
                    if (needsCompute && cmpIdx != i && families[i].supportsCompute()) cmpIdx = i;
                    if (surface && srfIdx != i && srfIdx != gfxIdx && m_physicalDevice->canPresentToSurface(surface, families[i])) srfIdx = i;
                }
                
                if (surface && srfIdx == -1 && m_physicalDevice->canPresentToSurface(surface, families[i])) {
                    srfIdx = i;
                    if (needsGraphics && gfxIdx != i && families[i].supportsGraphics()) gfxIdx = i;
                    if (needsCompute && cmpIdx != i && families[i].supportsCompute()) cmpIdx = i;
                    if (needsTransfer && xfrIdx != i && families[i].supportsTransfer()) xfrIdx = i;
                }

                gfxSatisfied = !needsGraphics || gfxIdx != -1;
                cmpSatisfied = !needsCompute || cmpIdx != -1;
                xfrSatisfied = !needsTransfer || xfrIdx != -1;
                srfSatisfied = !surface || srfIdx != -1;
                
                if (gfxSatisfied && cmpSatisfied && xfrSatisfied && srfSatisfied) break;
            }
                
            if (!gfxSatisfied || !cmpSatisfied || !xfrSatisfied || !srfSatisfied) return false;

            i32 addedIndices[4] = { -1, -1, -1, -1 };
            static f32 priority = 1.0f;
            u32 count = 0;
            
            if (needsGraphics) {
                infos[count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                infos[count].queueFamilyIndex = gfxIdx;
                infos[count].queueCount = 1;
                infos[count].pQueuePriorities = &priority;
                count++;

                *outGfxQueueInfoIdx = gfxIdx;
            }
            
            if (needsCompute) {
                bool alreadyAdded = false;
                for (u32 i = 0;i < count;i++) {
                    if (infos[i].queueFamilyIndex == cmpIdx) {
                        alreadyAdded = true;
                        break;
                    }
                }

                if (!alreadyAdded) {
                    infos[count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    infos[count].queueFamilyIndex = cmpIdx;
                    infos[count].queueCount = 1;
                    infos[count].pQueuePriorities = &priority;
                    count++;
                }

                *outCmpQueueInfoIdx = i32(cmpIdx);
            }
            
            if (needsTransfer) {
                bool alreadyAdded = false;
                for (u32 i = 0;i < count;i++) {
                    if (infos[i].queueFamilyIndex == xfrIdx) {
                        alreadyAdded = true;
                        break;
                    }
                }

                if (!alreadyAdded) {
                    infos[count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    infos[count].queueFamilyIndex = xfrIdx;
                    infos[count].queueCount = 1;
                    infos[count].pQueuePriorities = &priority;
                    count++;
                }
            }
            
            if (surface) {
                bool alreadyAdded = false;
                for (u32 i = 0;i < count;i++) {
                    if (infos[i].queueFamilyIndex == srfIdx) {
                        alreadyAdded = true;
                        break;
                    }
                }

                if (!alreadyAdded) {
                    infos[count].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    infos[count].queueFamilyIndex = srfIdx;
                    infos[count].queueCount = 1;
                    infos[count].pQueuePriorities = &priority;
                    count++;
                }

                *outSurfaceQueueInfoIdx = i32(srfIdx);
            }

            return true;
        }
    };
};