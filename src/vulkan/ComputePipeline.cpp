#include <render/vulkan/ComputePipeline.h>
#include <render/vulkan/ShaderCompiler.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Instance.h>

#include <utils/Array.hpp>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/Logger.h>

namespace render {
    namespace vulkan {
        ComputePipeline::ComputePipeline(
            ShaderCompiler* compiler,
            LogicalDevice* device
        ) : Pipeline(device), ::utils::IWithLogging("Compute Pipeline") {
            m_device =  device;
            m_compiler = compiler;
            m_layout = VK_NULL_HANDLE;
            m_pipeline = VK_NULL_HANDLE;
            m_descriptorSetLayout = VK_NULL_HANDLE;
            m_computeShader = nullptr;
        }

        ComputePipeline::~ComputePipeline() {
            shutdown();
        }

        
        bool ComputePipeline::setComputeShader(const String& source) {
            if (m_computeShader) return false;
            m_computeShader = m_compiler->compileShader(source, EShLangCompute);

            if (m_computeShader == nullptr) return false;
            m_computeShaderSrc = source;
            return true;
        }

        void ComputePipeline::addUniformBlock(u32 bindIndex) {
            m_uniformBlockBindings.push(bindIndex);
        }

        void ComputePipeline::addStorageBuffer(u32 bindIndex) {
            m_storageBufferBindings.push(bindIndex);
        }

        bool ComputePipeline::init() {
            if (m_pipeline || !m_device || !m_compiler) return false;

            EShMessages messageFlags = EShMessages(EShMsgSpvRules | EShMsgVulkanRules);

            glslang::TProgram prog;
            if (m_computeShader) prog.addShader(m_computeShader);

            if (!prog.link(messageFlags)) {
                error(String(prog.getInfoLog()));
                return false;
            }

            Array<VkPipelineShaderStageCreateInfo> stages;
            if (m_computeShader && !processShader(prog, EShLangCompute, stages)) { shutdown(); return false; }

            Array<VkDescriptorSetLayoutBinding> descriptorSetBindings;
            for (u32 i = 0;i < m_uniformBlockBindings.size();i++) {
                descriptorSetBindings.push({});
                auto& b = descriptorSetBindings.last();
                b.binding = m_uniformBlockBindings[i];
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.descriptorCount = 1;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.pImmutableSamplers = VK_NULL_HANDLE;
            }

            for (u32 i = 0;i < m_storageBufferBindings.size();i++) {
                descriptorSetBindings.push({});
                auto& b = descriptorSetBindings.last();
                b.binding = m_storageBufferBindings[i];
                b.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                b.descriptorCount = 1;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.pImmutableSamplers = VK_NULL_HANDLE;
            }

            VkDescriptorSetLayoutCreateInfo dsl = {};
            dsl.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            dsl.bindingCount = descriptorSetBindings.size();
            dsl.pBindings = descriptorSetBindings.data();

            if (vkCreateDescriptorSetLayout(m_device->get(), &dsl, m_device->getInstance()->getAllocator(), &m_descriptorSetLayout) != VK_SUCCESS) {
                error("Failed to create descriptor set layout");
                shutdown();
                return false;
            }

            VkPipelineLayoutCreateInfo li = {};
            li.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            li.setLayoutCount = 1;
            li.pSetLayouts = &m_descriptorSetLayout;
            li.pushConstantRangeCount = 0;
            li.pPushConstantRanges = nullptr;

            if (vkCreatePipelineLayout(m_device->get(), &li, m_device->getInstance()->getAllocator(), &m_layout) != VK_SUCCESS) {
                error("Failed to create compute pipeline layout");
                shutdown();
                return false;
            }

            VkComputePipelineCreateInfo pi = {};
            pi.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pi.layout = m_layout;
            pi.stage = stages[0];

            if (vkCreateComputePipelines(m_device->get(), VK_NULL_HANDLE, 1, &pi, m_device->getInstance()->getAllocator(), &m_pipeline) != VK_SUCCESS) {
                error("Failed to create compute pipeline");
                shutdown();
                return false;
            }

            return true;
        }

        void ComputePipeline::shutdown() {
            m_shaderModules.each([this](VkShaderModule mod) {
                vkDestroyShaderModule(m_device->get(), mod, m_device->getInstance()->getAllocator());
            });
            m_shaderModules.clear();

            if (m_layout) vkDestroyPipelineLayout(m_device->get(), m_layout, m_device->getInstance()->getAllocator());
            if (m_descriptorSetLayout) vkDestroyDescriptorSetLayout(m_device->get(), m_descriptorSetLayout, m_device->getInstance()->getAllocator());
            if (m_pipeline) vkDestroyPipeline(m_device->get(), m_pipeline, m_device->getInstance()->getAllocator());

            if (m_computeShader) delete m_computeShader;
            m_computeShader = nullptr;

            m_layout = VK_NULL_HANDLE;
            m_descriptorSetLayout = VK_NULL_HANDLE;
            m_pipeline = VK_NULL_HANDLE;
        }
        
        bool ComputePipeline::recreate() {
            shutdown();

            // glslang::TShader can't be reused...
            if (m_computeShaderSrc.size() > 0 && !setComputeShader(m_computeShaderSrc)) return false;

            return init();
        }

        VkPipeline ComputePipeline::get() const {
            return m_pipeline;
        }

        VkPipelineLayout ComputePipeline::getLayout() const {
            return m_layout;
        }

        VkDescriptorSetLayout ComputePipeline::getDescriptorSetLayout() const {
            return m_descriptorSetLayout;
        }
        
        bool ComputePipeline::processShader(
            glslang::TProgram& prog,
            EShLanguage lang,
            Array<VkPipelineShaderStageCreateInfo>& stages
        ) {
            glslang::TIntermediate& IR = *(prog.getIntermediate(lang));
            std::vector<u32> code;
            
            glslang::SpvOptions options = {};
            options.validate = true;

            spv::SpvBuildLogger logger;
            glslang::GlslangToSpv(IR, code, &logger, &options);

            String allLogs = logger.getAllMessages();
            auto msgs = allLogs.split("\n");
            for (u32 i = 0;i < msgs.size();i++) {
                String& ln = msgs[i];

                i64 idx = -1;
                
                idx = ln.firstIndexOf("TBD functionality: ");
                if (idx > 0) {
                    log(ln);
                    continue;
                }
                
                idx = ln.firstIndexOf("Missing functionality: ");
                if (idx > 0) {
                    log(ln);
                    continue;
                }
        
                idx = ln.firstIndexOf("warning: ");
                if (idx > 0) {
                    constexpr size_t len = sizeof("warning: ");
                    warn(String(&ln[len], ln.size() - len));
                    continue;
                }

                idx = ln.firstIndexOf("error: ");
                if (idx > 0) {
                    constexpr size_t len = sizeof("error: ");
                    error(String(&ln[len], ln.size() - len));
                    continue;
                }
            }
        
            if (code.size() == 0) return false;

            VkShaderModuleCreateInfo ci = {};
            ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            ci.codeSize = code.size() * sizeof(u32);
            ci.pCode = code.data();

            VkShaderModule mod = VK_NULL_HANDLE;
            VkResult result = vkCreateShaderModule(
                m_device->get(),
                &ci,
                m_device->getInstance()->getAllocator(),
                &mod
            );

            if (result != VK_SUCCESS) return false;

            VkPipelineShaderStageCreateInfo si = {};
            si.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            si.module = mod;
            si.pName = "main";

            switch (lang) {
                case EShLangVertex: { si.stage = VK_SHADER_STAGE_VERTEX_BIT; break; }
                case EShLangTessControl: { si.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; break; }
                case EShLangTessEvaluation: { si.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; break; }
                case EShLangGeometry: { si.stage = VK_SHADER_STAGE_GEOMETRY_BIT; break; }
                case EShLangFragment: { si.stage = VK_SHADER_STAGE_FRAGMENT_BIT; break; }
                case EShLangCompute: { si.stage = VK_SHADER_STAGE_COMPUTE_BIT; break; }
                case EShLangRayGen: { si.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR; break; }
                case EShLangIntersect: { si.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR; break; }
                case EShLangAnyHit: { si.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR; break; }
                case EShLangClosestHit: { si.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR; break; }
                case EShLangMiss: { si.stage = VK_SHADER_STAGE_MISS_BIT_KHR; break; }
                case EShLangCallable: { si.stage = VK_SHADER_STAGE_CALLABLE_BIT_KHR; break; }
                case EShLangTask: { si.stage = VK_SHADER_STAGE_TASK_BIT_EXT; break; }
                case EShLangMesh: { si.stage = VK_SHADER_STAGE_MESH_BIT_EXT; break; }
            }

            m_shaderModules.push(mod);
            stages.push(si);

            return true;
        }
    };
};