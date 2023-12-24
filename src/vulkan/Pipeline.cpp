#include <render/vulkan/Pipeline.h>

namespace render {
    namespace vulkan {
        Pipeline::Pipeline(LogicalDevice* device) {
            m_device = device;
            m_layout = VK_NULL_HANDLE;
            m_pipeline = VK_NULL_HANDLE;
            m_descriptorSetLayout = VK_NULL_HANDLE;
        }

        Pipeline::~Pipeline() {
        }

        VkPipeline Pipeline::get() const {
            return m_pipeline;
        }

        VkPipelineLayout Pipeline::getLayout() const {
            return m_layout;
        }

        VkDescriptorSetLayout Pipeline::getDescriptorSetLayout() const {
            return m_descriptorSetLayout;
        }
    };
};