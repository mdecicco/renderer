#include <render/vulkan/Pipeline.h>
#include <render/vulkan/ShaderCompiler.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/SwapChain.h>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/Logger.h>

namespace render {
    namespace vulkan {
        Pipeline::Pipeline(ShaderCompiler* compiler, LogicalDevice* device, SwapChain* swapChain) : utils::IWithLogging("Vulkan Pipeline") {
            m_compiler = compiler;
            m_device = device;
            m_swapChain = swapChain;
            m_layout = VK_NULL_HANDLE;
            m_renderPass = VK_NULL_HANDLE;
            m_pipeline = VK_NULL_HANDLE;
            m_vertexShader = m_fragShader = m_geomShader = m_computeShader = nullptr;
            m_isInitialized = false;

            reset();
        }

        Pipeline::~Pipeline() {
            shutdown();
        }

        void Pipeline::reset() {
            shutdown();

            m_scissorIsSet = false;
            setViewport(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f);
            m_viewportDynamic = false;
            m_scissorDynamic = false;

            m_primType               = PT_TRIANGLES;        m_primTypeDynamic               = false;
            m_polyMode               = PM_FILLED;           m_polyModeDynamic               = false;
            m_cullMode               = CM_BACK_FACE;        m_cullModeDynamic               = false;
            m_frontFaceMode          = FFM_CLOCKWISE;       m_frontFaceModeDynamic          = false;
            m_lineWidth              = 1.0f;                m_lineWidthDynamic              = false;
            m_primRestartEnabled     = false;               m_primRestartEnabledDynamic     = false;
            m_depthClampEnabled      = false;               m_depthClampEnabledDynamic      = false;
            m_depthCompareOp         = CO_GREATER;          m_depthCompareOpDynamic         = false;
            m_srcColorBlendFactor    = BF_ONE;              
            m_dstColorBlendFactor    = BF_ZERO;             
            m_colorBlendOp           = BO_ADD;              
            m_srcAlphaBlendFactor    = BF_ONE;              
            m_dstAlphaBlendFactor    = BF_ZERO;             
            m_alphaBlendOp           = BO_ADD;              m_blendEquationDynamic          = false;
            m_minDepthBounds         = 0.0f;
            m_maxDepthBounds         = 1.0f;                m_depthBoundsDynamic            = false;
            m_depthTestEnabled       = false;               m_depthTestEnabledDynamic       = false;
            m_depthBoundsTestEnabled = false;               m_depthBoundsTestEnabledDynamic = false;
            m_depthWriteEnabled      = false;               m_depthWriteEnabledDynamic      = false;
            m_colorBlendEnabled      = false;               m_colorBlendEnabledDynamic      = false;

            m_vertexShader = nullptr;
            m_fragShader = nullptr;
            m_geomShader = nullptr;
            m_computeShader = nullptr;
        }

        bool Pipeline::setVertexShader(const utils::String& source) {
            if (m_vertexShader) return false;
            m_vertexShader = m_compiler->compileShader(source, EShLangVertex);
            return m_vertexShader != nullptr;
        }

        bool Pipeline::setFragmentShader(const utils::String& source) {
            if (m_fragShader) return false;
            m_fragShader = m_compiler->compileShader(source, EShLangFragment);
            return m_fragShader != nullptr;
        }

        bool Pipeline::setGeometryShader(const utils::String& source) {
            if (m_geomShader) return false;
            m_geomShader = m_compiler->compileShader(source, EShLangGeometry);
            return m_geomShader != nullptr;
        }

        bool Pipeline::setComputeShader(const utils::String& source) {
            if (m_computeShader) return false;
            m_computeShader = m_compiler->compileShader(source, EShLangCompute);
            return m_computeShader != nullptr;
        }
        
        void Pipeline::setViewport(f32 x, f32 y, f32 w, f32 h, f32 minZ, f32 maxZ) {
            if (m_isInitialized && !m_viewportDynamic) return;

            m_viewport.x = x;
            m_viewport.y = y;
            m_viewport.width = w;
            m_viewport.height = h;
            m_viewport.minDepth = minZ;
            m_viewport.maxDepth = maxZ;

            if (!m_scissorIsSet) {
                m_scissor.offset.x = x;
                m_scissor.offset.y = x;
                m_scissor.extent.width = w;
                m_scissor.extent.height = h;
            }

            if (m_isInitialized) {
                // todo
            }
        }

        void Pipeline::setScissor(f32 x, f32 y, f32 w, f32 h) {
            if (m_isInitialized && !m_scissorDynamic) return;
            m_scissorIsSet = true;

            m_scissor.offset.x = x;
            m_scissor.offset.y = x;
            m_scissor.extent.width = w;
            m_scissor.extent.height = h;

            if (m_isInitialized) {
                // todo
            }
        }

        void Pipeline::addDynamicState(VkDynamicState state) {
            bool exists = m_dynamicState.some([state](VkDynamicState s) { return s == state; });
            if (exists) return;

            m_dynamicState.push(state);

            switch (state) {
                case VK_DYNAMIC_STATE_VIEWPORT: m_viewportDynamic = true; break;
                case VK_DYNAMIC_STATE_SCISSOR: m_scissorDynamic = true; break;
                case VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY: m_primTypeDynamic = true; break;
                case VK_DYNAMIC_STATE_POLYGON_MODE_EXT: m_polyModeDynamic = true; break;
                case VK_DYNAMIC_STATE_CULL_MODE: m_cullModeDynamic = true; break;
                case VK_DYNAMIC_STATE_FRONT_FACE: m_frontFaceModeDynamic = true; break;
                case VK_DYNAMIC_STATE_LINE_WIDTH: m_lineWidthDynamic = true; break;
                case VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE: m_primRestartEnabledDynamic = true; break;
                case VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT: m_depthClampEnabledDynamic = true; break;
                case VK_DYNAMIC_STATE_DEPTH_COMPARE_OP: m_depthCompareOpDynamic = true; break;
                case VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT: m_blendEquationDynamic = true; break;
                case VK_DYNAMIC_STATE_DEPTH_BOUNDS: m_depthBoundsDynamic = true; break;
                case VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE: m_depthTestEnabledDynamic = true; break;
                case VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE: m_depthBoundsTestEnabledDynamic = true; break;
                case VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE: m_depthWriteEnabledDynamic = true; break;
                case VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT: m_colorBlendEnabledDynamic = true; break;
                default: break;
            }
        }
        
        void Pipeline::setPrimitiveType(PRIMITIVE_TYPE ptype) {
            if (m_isInitialized && !m_primTypeDynamic) return;
            m_primType = ptype;
        }
        
        void Pipeline::setPrimitiveRestart(bool enabled) {
            if (m_isInitialized && !m_primRestartEnabledDynamic) return;
            m_primRestartEnabled = enabled;
        }
        
        void Pipeline::setPolygonMode(POLYGON_MODE pmode) {
            if (m_isInitialized && !m_polyModeDynamic) return;
            m_polyMode = pmode;
        }

        void Pipeline::setDepthClamp(bool enabled) {
            if (m_isInitialized && !m_depthClampEnabledDynamic) return;
            m_depthClampEnabled = enabled;
        }

        void Pipeline::setLineWidth(f32 width){
            if (m_isInitialized && !m_lineWidthDynamic) return;
            m_lineWidth = width;
        }

        void Pipeline::setCullMode(CULL_MODE cmode) {
            if (m_isInitialized && !m_cullModeDynamic) return;
            m_cullMode = cmode;
        }

        void Pipeline::setFrontFaceMode(FRONT_FACE_MODE ffmode) {
            if (m_isInitialized && !m_frontFaceModeDynamic) return;
            m_frontFaceMode = ffmode;
        }

        void Pipeline::setDepthCompareOp(COMPARE_OP op) {
            if (m_isInitialized && !m_depthCompareOpDynamic) return;
            m_depthCompareOp = op;
        }

        void Pipeline::setDepthBounds(f32 min, f32 max) {
            if (m_isInitialized && !m_depthBoundsDynamic) return;
            m_minDepthBounds = min;
            m_maxDepthBounds = max;
        }

        void Pipeline::setDepthTestEnabled(bool enabled) {
            if (m_isInitialized && !m_depthTestEnabledDynamic) return;
            m_depthTestEnabled = enabled;
        }

        void Pipeline::setDepthBoundsTestEnabled(bool enabled) {
            if (m_isInitialized && !m_depthBoundsTestEnabledDynamic) return;
            m_depthBoundsTestEnabled = enabled;
        }

        void Pipeline::setDepthWriteEnabled(bool enabled) {
            if (m_isInitialized && !m_depthWriteEnabledDynamic) return;
            m_depthWriteEnabled = enabled;
        }

        void Pipeline::setSrcColorBlendFactor(BLEND_FACTOR factor) {
            if (m_isInitialized && !m_blendEquationDynamic) return;
            m_srcColorBlendFactor = factor;
        }

        void Pipeline::setDstColorBlendFactor(BLEND_FACTOR factor) {
            if (m_isInitialized && !m_blendEquationDynamic) return;
            m_dstColorBlendFactor = factor;
        }

        void Pipeline::setColorBlendOp(BLEND_OP op) {
            if (m_isInitialized && !m_blendEquationDynamic) return;
            m_colorBlendOp = op;
        }

        void Pipeline::setSrcAlphaBlendFactor(BLEND_FACTOR factor) {
            if (m_isInitialized && !m_blendEquationDynamic) return;
            m_srcAlphaBlendFactor = factor;
        }

        void Pipeline::setDstAlphaBlendFactor(BLEND_FACTOR factor) {
            if (m_isInitialized && !m_blendEquationDynamic) return;
            m_dstAlphaBlendFactor = factor;
        }

        void Pipeline::setAlphaBlendOp(BLEND_OP op) {
            if (m_isInitialized && !m_blendEquationDynamic) return;
            m_alphaBlendOp = op;
        }

        void Pipeline::setColorBlendEnabled(bool enabled) {
            if (m_isInitialized && !m_colorBlendEnabledDynamic) return;
            m_colorBlendEnabled = enabled;
        }

        bool Pipeline::init() {
            if (m_isInitialized || !m_device || !m_swapChain || !m_compiler) return false;

            EShMessages messageFlags = EShMessages(EShMsgSpvRules | EShMsgVulkanRules);

            glslang::TProgram prog;
            if (m_vertexShader) prog.addShader(m_vertexShader);
            if (m_fragShader) prog.addShader(m_fragShader);
            if (m_geomShader) prog.addShader(m_geomShader);
            if (m_computeShader) prog.addShader(m_computeShader);

            if (!prog.link(messageFlags)) {
                error(utils::String(prog.getInfoLog()));
                return false;
            }

            utils::Array<VkPipelineShaderStageCreateInfo> stages;
            if (m_vertexShader && !processShader(prog, EShLangVertex, stages)) { shutdown(); return false; }
            if (m_fragShader && !processShader(prog, EShLangFragment, stages)) { shutdown(); return false; }
            if (m_geomShader && !processShader(prog, EShLangGeometry, stages)) { shutdown(); return false; }
            if (m_computeShader && !processShader(prog, EShLangCompute, stages)) { shutdown(); return false; }

            VkPipelineDynamicStateCreateInfo di = {};
            di.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            di.dynamicStateCount = m_dynamicState.size();
            di.pDynamicStates = m_dynamicState.data();

            VkPipelineVertexInputStateCreateInfo vi = {};
            vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vi.vertexBindingDescriptionCount = m_vertexBindings.size();
            vi.pVertexBindingDescriptions = m_vertexBindings.data();
            vi.vertexAttributeDescriptionCount = m_vertexAttribs.size();
            vi.pVertexAttributeDescriptions = m_vertexAttribs.data();

            VkPipelineInputAssemblyStateCreateInfo ai = {};
            ai.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            switch (m_primType) {
                case PT_POINTS: ai.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
                case PT_LINES: ai.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
                case PT_LINE_STRIP: ai.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; break;
                case PT_TRIANGLES: ai.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
                case PT_TRIANGLE_STRIP: ai.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
                case PT_TRIANGLE_FAN: ai.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
            }
            ai.primitiveRestartEnable = m_primRestartEnabled ? VK_TRUE : VK_FALSE;

            VkPipelineViewportStateCreateInfo vpi = {};
            vpi.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            vpi.viewportCount = 1;
            vpi.pViewports = &m_viewport;
            vpi.scissorCount = 1;
            vpi.pScissors = &m_scissor;

            VkPipelineRasterizationStateCreateInfo ri = {};
            ri.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            ri.depthClampEnable = m_depthClampEnabled ? VK_TRUE : VK_FALSE;
            ri.rasterizerDiscardEnable = VK_FALSE;
            ri.lineWidth = m_lineWidth;
            ri.depthBiasEnable = VK_FALSE;
            ri.depthBiasConstantFactor = 0.0f;
            ri.depthBiasClamp = 0.0f;
            ri.depthBiasSlopeFactor = 0.0f;
            
            switch (m_polyMode) {
                case PM_FILLED: ri.polygonMode = VK_POLYGON_MODE_FILL; break;
                case PM_WIREFRAME: ri.polygonMode = VK_POLYGON_MODE_LINE; break;
                case PM_POINTS: ri.polygonMode = VK_POLYGON_MODE_POINT; break;
            }

            switch (m_cullMode) {
                case CM_FRONT_FACE: ri.cullMode = VK_CULL_MODE_FRONT_BIT; break;
                case CM_BACK_FACE: ri.cullMode = VK_CULL_MODE_BACK_BIT; break;
                case CM_BOTH_FACES: ri.cullMode = VK_CULL_MODE_FRONT_AND_BACK; break;
            }

            switch (m_frontFaceMode) {
                case FFM_CLOCKWISE: ri.frontFace = VK_FRONT_FACE_CLOCKWISE; break;
                case FFM_COUNTER_CLOCKWISE: ri.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; break;
            }

            VkPipelineMultisampleStateCreateInfo msi = {};
            msi.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            msi.sampleShadingEnable = VK_FALSE;
            msi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            msi.minSampleShading = 1.0f;
            msi.pSampleMask = nullptr;
            msi.alphaToCoverageEnable = VK_FALSE;
            msi.alphaToOneEnable = VK_FALSE;

            VkPipelineDepthStencilStateCreateInfo dsi = {};
            dsi.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            dsi.depthBoundsTestEnable = m_depthBoundsTestEnabled ? VK_TRUE : VK_FALSE;
            dsi.depthTestEnable = m_depthTestEnabled ? VK_TRUE : VK_FALSE;
            dsi.depthWriteEnable = m_depthWriteEnabled ? VK_TRUE : VK_FALSE;
            dsi.minDepthBounds = m_minDepthBounds;
            dsi.maxDepthBounds = m_maxDepthBounds;
            dsi.stencilTestEnable = VK_FALSE;

            switch (m_depthCompareOp) {
                case CO_NEVER: dsi.depthCompareOp = VK_COMPARE_OP_NEVER; break;
                case CO_ALWAYS: dsi.depthCompareOp = VK_COMPARE_OP_ALWAYS; break;
                case CO_LESS: dsi.depthCompareOp = VK_COMPARE_OP_LESS; break;
                case CO_LESS_OR_EQUAL: dsi.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; break;
                case CO_GREATER: dsi.depthCompareOp = VK_COMPARE_OP_GREATER; break;
                case CO_GREATER_OR_EQUAL: dsi.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; break;
                case CO_EQUAL: dsi.depthCompareOp = VK_COMPARE_OP_EQUAL; break;
                case CO_NOT_EQUAL: dsi.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL; break;
            }

            auto blendFac = [](BLEND_FACTOR bf) {
                switch (bf) {
                    case BF_ZERO: return VK_BLEND_FACTOR_ZERO;
                    case BF_ONE: return VK_BLEND_FACTOR_ONE;
                    case BF_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
                    case BF_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                    case BF_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
                    case BF_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                    case BF_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
                    case BF_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                    case BF_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
                    case BF_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                    case BF_CONSTANT_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
                    case BF_ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
                    case BF_CONSTANT_ALPHA: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
                    case BF_ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
                    case BF_SRC_ALPHA_SATURATE: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
                    case BF_SRC1_COLOR: return VK_BLEND_FACTOR_SRC1_COLOR;
                    case BF_ONE_MINUS_SRC1_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
                    case BF_SRC1_ALPHA: return VK_BLEND_FACTOR_SRC1_ALPHA;
                    case BF_ONE_MINUS_SRC1_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
                }
            };

            auto blendOp = [](BLEND_OP bo) {
                switch (bo) {
                    case BO_ADD: return VK_BLEND_OP_ADD;
                    case BO_SUBTRACT: return VK_BLEND_OP_SUBTRACT;
                    case BO_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
                    case BO_MIN: return VK_BLEND_OP_MIN;
                    case BO_MAX: return VK_BLEND_OP_MAX;
                }
            };

            VkPipelineColorBlendAttachmentState cbai = {};
            cbai.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            cbai.blendEnable = m_colorBlendEnabled ? VK_TRUE : VK_FALSE;
            cbai.srcColorBlendFactor = blendFac(m_srcColorBlendFactor);
            cbai.dstColorBlendFactor = blendFac(m_dstColorBlendFactor);
            cbai.colorBlendOp = blendOp(m_colorBlendOp);
            cbai.srcAlphaBlendFactor = blendFac(m_srcAlphaBlendFactor);
            cbai.dstAlphaBlendFactor = blendFac(m_dstAlphaBlendFactor);
            cbai.alphaBlendOp = blendOp(m_alphaBlendOp);

            VkPipelineColorBlendStateCreateInfo cbsi = {};
            cbsi.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            cbsi.logicOpEnable = VK_FALSE;
            cbsi.logicOp = VK_LOGIC_OP_COPY;
            cbsi.attachmentCount = 1;
            cbsi.pAttachments = &cbai;
            cbsi.blendConstants[0] = 0.0f;
            cbsi.blendConstants[1] = 0.0f;
            cbsi.blendConstants[2] = 0.0f;
            cbsi.blendConstants[3] = 0.0f;

            VkPipelineLayoutCreateInfo li = {};
            li.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            li.setLayoutCount = 0;
            li.pSetLayouts = nullptr;
            li.pushConstantRangeCount = 0;
            li.pPushConstantRanges = nullptr;

            if (vkCreatePipelineLayout(m_device->get(), &li, m_device->getInstance()->getAllocator(), &m_layout) != VK_SUCCESS) {
                error("Failed to create pipeline layout");
                shutdown();
                return false;
            }

            VkAttachmentDescription ca = {};
            ca.format = m_swapChain->getFormat();
            ca.samples = VK_SAMPLE_COUNT_1_BIT;
            ca.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            ca.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            ca.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            ca.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            ca.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference ar = {};
            ar.attachment = 0;
            ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription sd = {};
            sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            sd.colorAttachmentCount = 1;
            sd.pColorAttachments = &ar;

            VkRenderPassCreateInfo rpi = {};
            rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            rpi.attachmentCount = 1;
            rpi.pAttachments = &ca;
            rpi.subpassCount = 1;
            rpi.pSubpasses = &sd;

            if (vkCreateRenderPass(m_device->get(), &rpi, m_device->getInstance()->getAllocator(), &m_renderPass) != VK_SUCCESS) {
                error("Failed to create pipeline layout");
                shutdown();
                return false;
            }

            VkGraphicsPipelineCreateInfo pci = {};
            pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pci.stageCount = stages.size();
            pci.pStages = stages.data();
            pci.pVertexInputState = &vi;
            pci.pInputAssemblyState = &ai;
            pci.pViewportState = &vpi;
            pci.pRasterizationState = &ri;
            pci.pMultisampleState = &msi;
            pci.pDepthStencilState = &dsi;
            pci.pColorBlendState = &cbsi;
            pci.pDynamicState = &di;
            pci.layout = m_layout;
            pci.renderPass = m_renderPass;
            pci.subpass = 0;
            pci.basePipelineHandle = VK_NULL_HANDLE;
            pci.basePipelineIndex = -1;

            if (vkCreateGraphicsPipelines(m_device->get(), nullptr, 1, &pci, m_device->getInstance()->getAllocator(), &m_pipeline) != VK_SUCCESS) {
                error("Failed to create graphics pipeline");
                shutdown();
                return false;
            }

            return true;
        }

        void Pipeline::shutdown() {
            m_shaderModules.each([this](VkShaderModule mod) {
                vkDestroyShaderModule(m_device->get(), mod, m_device->getInstance()->getAllocator());
            });
            m_shaderModules.clear();

            if (m_layout) vkDestroyPipelineLayout(m_device->get(), m_layout, m_device->getInstance()->getAllocator());
            if (m_renderPass) vkDestroyRenderPass(m_device->get(), m_renderPass, m_device->getInstance()->getAllocator());
            if (m_pipeline) vkDestroyPipeline(m_device->get(), m_pipeline, m_device->getInstance()->getAllocator());

            m_layout = VK_NULL_HANDLE;
            m_renderPass = VK_NULL_HANDLE;
            m_pipeline = VK_NULL_HANDLE;
            m_isInitialized = false;
        }

        bool Pipeline::processShader(
            glslang::TProgram& prog,
            EShLanguage lang,
            utils::Array<VkPipelineShaderStageCreateInfo>& stages
        ) {
            glslang::TIntermediate& IR = *(prog.getIntermediate(lang));
            std::vector<u32> code;
            
            glslang::SpvOptions options = {};
            options.validate = true;

            spv::SpvBuildLogger logger;
            glslang::GlslangToSpv(IR, code, &logger, &options);

            utils::String allLogs = logger.getAllMessages();
            auto msgs = allLogs.split("\n");
            for (u32 i = 0;i < msgs.size();i++) {
                utils::String& ln = msgs[i];

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
                    warn(utils::String(&ln[len], ln.size() - len));
                    continue;
                }

                idx = ln.firstIndexOf("error: ");
                if (idx > 0) {
                    constexpr size_t len = sizeof("error: ");
                    error(utils::String(&ln[len], ln.size() - len));
                    continue;
                }
            }
        
            if (code.size() == 0) return false;

            VkShaderModuleCreateInfo ci = {};
            ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            ci.codeSize = code.size();
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
            si.stage = VK_SHADER_STAGE_VERTEX_BIT;
            si.module = mod;
            si.pName = "main";

            m_shaderModules.push(mod);
            stages.push(si);

            return true;
        }
    };
};