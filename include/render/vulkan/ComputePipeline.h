#pragma once
#include <render/types.h>
#include <render/vulkan/Pipeline.h>

#include <utils/ILogListener.h>
#include <vulkan/vulkan.h>
#include <glslang/Public/ShaderLang.h>

namespace render {
    namespace core {
        class DataFormat;
    };

    namespace vulkan {
        class ShaderCompiler;
        class LogicalDevice;

        class ComputePipeline : public Pipeline, public ::utils::IWithLogging {
            public:
                ComputePipeline(ShaderCompiler* compiler, LogicalDevice* device);
                virtual ~ComputePipeline();

                bool setComputeShader(const String& source);
                void addUniformBlock(u32 bindIndex);
                void addStorageBuffer(u32 bindIndex);

                bool init();
                void shutdown();
                bool recreate();

                VkPipeline get() const;
                VkPipelineLayout getLayout() const;
                VkDescriptorSetLayout getDescriptorSetLayout() const;

            protected:
                bool processShader(
                    glslang::TProgram& prog,
                    EShLanguage lang,
                    Array<VkPipelineShaderStageCreateInfo>& stages
                );

                ShaderCompiler* m_compiler;
                LogicalDevice* m_device;
                Array<u32> m_uniformBlockBindings;
                Array<u32> m_storageBufferBindings;

                String m_computeShaderSrc;
                glslang::TShader* m_computeShader;

                Array<VkShaderModule> m_shaderModules;
        };
    };
};