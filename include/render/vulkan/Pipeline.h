#pragma once
#include <render/types.h>
#include <render/core/VertexFormat.h>

#include <utils/Array.h>
#include <utils/ILogListener.h>

#include <vulkan/vulkan.h>
#include <glslang/Public/ShaderLang.h>

namespace render {
    namespace vulkan {
        class ShaderCompiler;
        class LogicalDevice;
        class SwapChain;

        class Pipeline : public utils::IWithLogging {
            public:
                Pipeline(ShaderCompiler* compiler, LogicalDevice* device, SwapChain* swapChain);
                ~Pipeline();

                void reset();

                void setVertexFormat(const core::VertexFormat& fmt);
                bool setVertexShader(const utils::String& source);
                bool setFragmentShader(const utils::String& source);
                bool setGeometryShader(const utils::String& source);
                bool setComputeShader(const utils::String& source);
                void addDynamicState(VkDynamicState state);

                void setViewport(f32 x, f32 y, f32 w, f32 h, f32 minZ, f32 maxZ);
                void setScissor(f32 x, f32 y, f32 w, f32 h);
                void setPrimitiveType(PRIMITIVE_TYPE ptype);
                void setPrimitiveRestart(bool enabled);
                void setPolygonMode(POLYGON_MODE pmode);
                void setDepthClamp(bool enabled);
                void setLineWidth(f32 width);
                void setCullMode(CULL_MODE cmode);
                void setFrontFaceMode(FRONT_FACE_MODE ffmode);
                void setDepthCompareOp(COMPARE_OP op);
                void setDepthBounds(f32 min, f32 max);
                void setDepthTestEnabled(bool enabled);
                void setDepthBoundsTestEnabled(bool enabled);
                void setDepthWriteEnabled(bool enabled);
                void setSrcColorBlendFactor(BLEND_FACTOR factor);
                void setDstColorBlendFactor(BLEND_FACTOR factor);
                void setColorBlendOp(BLEND_OP op);
                void setSrcAlphaBlendFactor(BLEND_FACTOR factor);
                void setDstAlphaBlendFactor(BLEND_FACTOR factor);
                void setAlphaBlendOp(BLEND_OP op);
                void setColorBlendEnabled(bool enabled);

                bool init();
                void shutdown();
                bool recreate();

                VkPipeline get() const;
                VkRenderPass getRenderPass() const;
                SwapChain* getSwapChain() const;
                const utils::Array<VkFramebuffer>& getFramebuffers() const;

            protected:
                bool processShader(
                    glslang::TProgram& prog,
                    EShLanguage lang,
                    utils::Array<VkPipelineShaderStageCreateInfo>& stages
                );

                ShaderCompiler* m_compiler;
                LogicalDevice* m_device;
                SwapChain* m_swapChain;
                VkPipelineLayout m_layout;
                VkRenderPass m_renderPass;
                VkPipeline m_pipeline;
                core::VertexFormat m_vertexFormat;
                bool m_isInitialized;
                bool m_scissorIsSet;

                utils::String m_vertexShaderSrc;
                glslang::TShader* m_vertexShader;
                utils::String m_fragShaderSrc;
                glslang::TShader* m_fragShader;
                utils::String m_geomShaderSrc;
                glslang::TShader* m_geomShader;
                utils::String m_computeShaderSrc;
                glslang::TShader* m_computeShader;

                utils::Array<VkShaderModule> m_shaderModules;
                utils::Array<VkDynamicState> m_dynamicState;
                utils::Array<VkFramebuffer> m_framebuffers;

                // state
                VkViewport m_viewport;
                VkRect2D m_scissor;
                PRIMITIVE_TYPE m_primType;
                POLYGON_MODE m_polyMode;
                CULL_MODE m_cullMode;
                FRONT_FACE_MODE m_frontFaceMode;
                COMPARE_OP m_depthCompareOp;
                BLEND_FACTOR m_srcColorBlendFactor;
                BLEND_FACTOR m_dstColorBlendFactor;
                BLEND_FACTOR m_srcAlphaBlendFactor;
                BLEND_FACTOR m_dstAlphaBlendFactor;
                BLEND_OP m_colorBlendOp;
                BLEND_OP m_alphaBlendOp;
                f32 m_minDepthBounds;
                f32 m_maxDepthBounds;
                f32 m_lineWidth;
                bool m_primRestartEnabled;
                bool m_depthClampEnabled;
                bool m_depthTestEnabled;
                bool m_depthBoundsTestEnabled;
                bool m_depthWriteEnabled;
                bool m_colorBlendEnabled;

                bool m_viewportDynamic;
                bool m_scissorDynamic;
                bool m_primTypeDynamic;
                bool m_polyModeDynamic;
                bool m_cullModeDynamic;
                bool m_frontFaceModeDynamic;
                bool m_depthCompareOpDynamic;
                bool m_blendEquationDynamic;
                bool m_depthBoundsDynamic;
                bool m_lineWidthDynamic;
                bool m_primRestartEnabledDynamic;
                bool m_depthClampEnabledDynamic;
                bool m_depthTestEnabledDynamic;
                bool m_depthBoundsTestEnabledDynamic;
                bool m_depthWriteEnabledDynamic;
                bool m_colorBlendEnabledDynamic;
        };
    };
};