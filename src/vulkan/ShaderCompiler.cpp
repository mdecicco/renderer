#include <render/vulkan/ShaderCompiler.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/PhysicalDevice.h>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>

namespace render {
    namespace vulkan {
        TBuiltInResource DefaultResources(LogicalDevice* device) {
            TBuiltInResource Resources;

            auto& props = device->getPhysicalDevice()->getProperties();
            auto& limits = props.limits;

            Resources.maxLights                                   = 32;
            Resources.maxClipPlanes                               = 6;
            Resources.maxTextureUnits                             = 32;
            Resources.maxTextureCoords                            = 32;
            Resources.maxVertexAttribs                            = 64;
            Resources.maxVertexUniformComponents                  = 4096;
            Resources.maxVaryingFloats                            = 64;
            Resources.maxVertexTextureImageUnits                  = 32;
            Resources.maxCombinedTextureImageUnits                = 80;
            Resources.maxTextureImageUnits                        = 32;
            Resources.maxFragmentUniformComponents                = 4096;
            Resources.maxDrawBuffers                              = i32(limits.maxColorAttachments);
            Resources.maxVertexUniformVectors                     = 128;
            Resources.maxVaryingVectors                           = 8;
            Resources.maxFragmentUniformVectors                   = 16;
            Resources.maxVertexOutputVectors                      = 16;
            Resources.maxFragmentInputVectors                     = 15;
            Resources.minProgramTexelOffset                       = -8;
            Resources.maxProgramTexelOffset                       = i32(limits.maxTexelOffset);
            Resources.maxClipDistances                            = i32(limits.maxClipDistances);
            Resources.maxComputeWorkGroupCountX                   = i32(limits.maxComputeWorkGroupCount[0]);
            Resources.maxComputeWorkGroupCountY                   = i32(limits.maxComputeWorkGroupCount[1]);
            Resources.maxComputeWorkGroupCountZ                   = i32(limits.maxComputeWorkGroupCount[2]);
            Resources.maxComputeWorkGroupSizeX                    = i32(limits.maxComputeWorkGroupSize[0]);
            Resources.maxComputeWorkGroupSizeY                    = i32(limits.maxComputeWorkGroupSize[1]);
            Resources.maxComputeWorkGroupSizeZ                    = i32(limits.maxComputeWorkGroupSize[2]);
            Resources.maxComputeUniformComponents                 = 1024;
            Resources.maxComputeTextureImageUnits                 = 16;
            Resources.maxComputeImageUniforms                     = 8;
            Resources.maxComputeAtomicCounters                    = 8;
            Resources.maxComputeAtomicCounterBuffers              = 1;
            Resources.maxVaryingComponents                        = 60;
            Resources.maxVertexOutputComponents                   = 64;
            Resources.maxGeometryInputComponents                  = i32(limits.maxGeometryInputComponents);
            Resources.maxGeometryOutputComponents                 = i32(limits.maxGeometryOutputComponents);
            Resources.maxFragmentInputComponents                  = i32(limits.maxFragmentInputComponents);
            Resources.maxImageUnits                               = 8;
            Resources.maxCombinedImageUnitsAndFragmentOutputs     = 8;
            Resources.maxCombinedShaderOutputResources            = 8;
            Resources.maxImageSamples                             = 0;
            Resources.maxVertexImageUniforms                      = 0;
            Resources.maxTessControlImageUniforms                 = 0;
            Resources.maxTessEvaluationImageUniforms              = 0;
            Resources.maxGeometryImageUniforms                    = 0;
            Resources.maxFragmentImageUniforms                    = 8;
            Resources.maxCombinedImageUniforms                    = 8;
            Resources.maxGeometryTextureImageUnits                = 16;
            Resources.maxGeometryOutputVertices                   = i32(limits.maxGeometryOutputVertices);
            Resources.maxGeometryTotalOutputComponents            = i32(limits.maxGeometryTotalOutputComponents);
            Resources.maxGeometryUniformComponents                = 1024;
            Resources.maxGeometryVaryingComponents                = 64;
            Resources.maxTessControlInputComponents               = i32(limits.maxTessellationControlPerVertexInputComponents);
            Resources.maxTessControlOutputComponents              = 128;
            Resources.maxTessControlTextureImageUnits             = 16;
            Resources.maxTessControlUniformComponents             = 1024;
            Resources.maxTessControlTotalOutputComponents         = i32(limits.maxTessellationControlTotalOutputComponents);
            Resources.maxTessEvaluationInputComponents            = i32(limits.maxTessellationEvaluationInputComponents);
            Resources.maxTessEvaluationOutputComponents           = i32(limits.maxTessellationEvaluationOutputComponents);;
            Resources.maxTessEvaluationTextureImageUnits          = 16;
            Resources.maxTessEvaluationUniformComponents          = 1024;
            Resources.maxTessPatchComponents                      = i32(limits.maxTessellationControlPerPatchOutputComponents);
            Resources.maxPatchVertices                            = i32(limits.maxTessellationPatchSize);
            Resources.maxTessGenLevel                             = i32(limits.maxTessellationGenerationLevel);
            Resources.maxViewports                                = i32(limits.maxViewports);
            Resources.maxVertexAtomicCounters                     = 0;
            Resources.maxTessControlAtomicCounters                = 0;
            Resources.maxTessEvaluationAtomicCounters             = 0;
            Resources.maxGeometryAtomicCounters                   = 0;
            Resources.maxFragmentAtomicCounters                   = 8;
            Resources.maxCombinedAtomicCounters                   = 8;
            Resources.maxAtomicCounterBindings                    = 1;
            Resources.maxVertexAtomicCounterBuffers               = 0;
            Resources.maxTessControlAtomicCounterBuffers          = 0;
            Resources.maxTessEvaluationAtomicCounterBuffers       = 0;
            Resources.maxGeometryAtomicCounterBuffers             = 0;
            Resources.maxFragmentAtomicCounterBuffers             = 1;
            Resources.maxCombinedAtomicCounterBuffers             = 1;
            Resources.maxAtomicCounterBufferSize                  = 16384;
            Resources.maxTransformFeedbackBuffers                 = 4;
            Resources.maxTransformFeedbackInterleavedComponents   = 64;
            Resources.maxCullDistances                            = i32(limits.maxCullDistances);
            Resources.maxCombinedClipAndCullDistances             = i32(limits.maxCombinedClipAndCullDistances);
            Resources.maxSamples                                  = 4;
            Resources.maxMeshOutputVerticesNV                     = 256;
            Resources.maxMeshOutputPrimitivesNV                   = 512;
            Resources.maxMeshWorkGroupSizeX_NV                    = 32;
            Resources.maxMeshWorkGroupSizeY_NV                    = 1;
            Resources.maxMeshWorkGroupSizeZ_NV                    = 1;
            Resources.maxTaskWorkGroupSizeX_NV                    = 32;
            Resources.maxTaskWorkGroupSizeY_NV                    = 1;
            Resources.maxTaskWorkGroupSizeZ_NV                    = 1;
            Resources.maxMeshViewCountNV                          = 4;
            Resources.limits.nonInductiveForLoops                 = 1;
            Resources.limits.whileLoops                           = 1;
            Resources.limits.doWhileLoops                         = 1;
            Resources.limits.generalUniformIndexing               = 1;
            Resources.limits.generalAttributeMatrixVectorIndexing = 1;
            Resources.limits.generalVaryingIndexing               = 1;
            Resources.limits.generalSamplerIndexing               = 1;
            Resources.limits.generalVariableIndexing              = 1;
            Resources.limits.generalConstantMatrixVectorIndexing  = 1;

            return Resources;
        }

        ShaderCompiler::ShaderCompiler(LogicalDevice* device) : utils::IWithLogging("Shader Compiler") {
            m_device = device;
            m_isInitialized = false;
        }

        ShaderCompiler::~ShaderCompiler() {
            shutdown();
        }

        bool ShaderCompiler::init() {
            m_isInitialized = glslang::InitializeProcess();
            return m_isInitialized;
        }

        void ShaderCompiler::shutdown() {
            if (!m_isInitialized) return;

            glslang::FinalizeProcess();
            m_isInitialized = false;
        }
        
        glslang::TShader* ShaderCompiler::compileShader(const String& source, EShLanguage type) {
            TBuiltInResource resources = DefaultResources(m_device);
            EShMessages messageFlags = EShMessages(EShMsgSpvRules | EShMsgVulkanRules);

            glslang::TShader* shdr = new glslang::TShader(type);
            const char* src = source.c_str();
            shdr->setStrings(&src, 1);
            shdr->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
            shdr->setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_6);
            shdr->setEntryPoint("main");

            glslang::TShader::ForbidIncluder noIncludes;
            std::string preprocessOutput;
            bool preprocessSuccess = shdr->preprocess(
                &resources,
                450,
                ENoProfile,
                false,
                false,
                messageFlags,
                &preprocessOutput,
                noIncludes
            );

            if (!preprocessSuccess) {
                error(String(shdr->getInfoLog()));
                delete shdr;
                return nullptr;
            }

            const char* preprocessedSrc = preprocessOutput.c_str();
            shdr->setStrings(&preprocessedSrc, 1);

            bool parseSuccess = shdr->parse(&resources, 450, false, messageFlags);
            if (!parseSuccess) {
                error(String(shdr->getInfoLog()));
                delete shdr;
                return nullptr;
            }

            return shdr;
        }
    };
};