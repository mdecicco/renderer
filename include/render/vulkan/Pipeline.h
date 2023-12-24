#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>

namespace render {
    namespace vulkan {
        class LogicalDevice;
        
        class Pipeline {
            public:
                Pipeline(LogicalDevice* device);
                virtual ~Pipeline();

                VkPipeline get() const;
                VkPipelineLayout getLayout() const;
                VkDescriptorSetLayout getDescriptorSetLayout() const;
            
            protected:
                LogicalDevice* m_device;
                VkPipelineLayout m_layout;
                VkDescriptorSetLayout m_descriptorSetLayout;
                VkPipeline m_pipeline;
        };
    };
};