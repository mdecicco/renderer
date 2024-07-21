#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        class Pipeline;
        class Texture;
        class UniformObject;
        class DescriptorSet;
        class Buffer;

        class DescriptorPool {
            public:
                DescriptorPool(LogicalDevice* device, u32 maxSets);
                ~DescriptorPool();

                LogicalDevice* getDevice() const;
                u32 getCapacity() const;
                u32 getRemaining() const;

                bool init();
                void shutdown();

                DescriptorSet* allocate(Pipeline* pipeline);
                void free(DescriptorSet* set);
            
            private:
                void resetNodes();
                DescriptorSet* getNode();
                void freeNode(DescriptorSet* n);

                LogicalDevice* m_device;
                u32 m_maxSets;
                u32 m_usedSets;
                VkDescriptorPool m_pool;

                DescriptorSet* m_freeList;
                DescriptorSet* m_usedList;
                DescriptorSet* m_sets;
        };

        class DescriptorSet {
            public:
                DescriptorPool* getPool() const;
                VkDescriptorSet get() const;

                void add(Texture* tex, u32 bindingIndex);
                void add(UniformObject* uo, u32 bindingIndex);
                void add(Buffer* storageBuffer, u32 bindingIndex);
                void set(Texture* tex, u32 bindingIndex);
                void set(UniformObject* uo, u32 bindingIndex);
                void set(Buffer* storageBuffer, u32 bindingIndex);
                void update();

                void free();

            private:
                friend class DescriptorPool;

                DescriptorSet();

                struct descriptor {
                    UniformObject* uniform;
                    Texture* tex;
                    Buffer* storageBuffer;
                    u32 bindingIdx;
                };

                Array<descriptor> m_descriptors;

                DescriptorPool* m_pool;
                DescriptorSet* m_next;
                DescriptorSet* m_last;

                VkDescriptorSet m_set;
        };

        class DescriptorFactory {
            public:
                DescriptorFactory(LogicalDevice* device, u32 maxSetsPerPool);
                ~DescriptorFactory();

                DescriptorSet* allocate(Pipeline* pipeline);
            
            private:
                LogicalDevice* m_device;
                u32 m_maxSetsPerPool;
                Array<DescriptorPool*> m_pools;
        };
    };
};