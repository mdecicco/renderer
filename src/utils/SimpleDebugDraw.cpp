#include <render/utils/SimpleDebugDraw.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/VertexBuffer.h>
#include <render/vulkan/UniformBuffer.h>
#include <render/vulkan/DescriptorSet.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/RenderPass.h>
#include <render/vulkan/Pipeline.h>

#include <utils/Array.hpp>

namespace render {
    namespace utils {
        SimpleDebugDraw::SimpleDebugDraw() {
            m_swapChain = nullptr;
            m_vboFactory = nullptr;
            m_uboFactory = nullptr;
            m_pipeline = nullptr;
            m_renderPass = nullptr;
            m_manualProjection = false;
            m_manualView = false;

            m_vfmt.addAttr(&vertex::position);
            m_vfmt.addAttr(&vertex::color);
            m_ufmt.addAttr(&uniforms::viewProj);
        }

        SimpleDebugDraw::~SimpleDebugDraw() {
            shutdown();
        }

        bool SimpleDebugDraw::init(
            vulkan::ShaderCompiler* compiler,
            vulkan::SwapChain* swapChain,
            vulkan::RenderPass* renderPass,
            vulkan::VertexBufferFactory* vboFactory,
            vulkan::UniformBufferFactory* uboFactory,
            vulkan::DescriptorFactory* dsFactory,
            u32 maxLines
        ) {
            m_swapChain = swapChain;
            m_renderPass = renderPass;
            m_vboFactory = vboFactory;
            m_uboFactory = uboFactory;
            m_dsFactory = dsFactory;
            m_maxLines = maxLines;

            m_pipeline = new vulkan::Pipeline(compiler, m_swapChain->getDevice(), m_swapChain, m_renderPass);
            m_pipeline->setVertexFormat(&m_vfmt);
            m_pipeline->addUniformBlock(0, &m_ufmt, VK_SHADER_STAGE_VERTEX_BIT);
            m_pipeline->addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
            m_pipeline->addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
            m_pipeline->setPrimitiveType(PT_LINES);
            m_pipeline->setDepthTestEnabled(true);
            m_pipeline->setDepthCompareOp(COMPARE_OP::CO_LESS_OR_EQUAL);
            m_pipeline->setDepthWriteEnabled(true);

            const char* vsh =
                "layout (location = 0) in vec3 v_pos;\n"
                "layout (location = 1) in vec4 v_color;\n"
                "layout (location = 0) out vec4 a_color;\n"
                "layout (binding = 0) uniform _ubo {\n"
                "    mat4 viewProj;\n"
                "} ubo;\n"
                "\n"
                "void main() {\n"
                "  gl_Position = ubo.viewProj * vec4(v_pos, 1.0);\n"
                "  a_color = v_color;\n"
                "}\n"
            ;

            const char* fsh =
                "layout (location = 0) in vec4 a_color;\n"
                "layout (location = 0) out vec4 o_color;\n"
                "\n"
                "void main() {\n"
                "    o_color = a_color;\n"
                "}\n"
            ;

            if (!m_pipeline->setVertexShader(vsh)) {
                shutdown();
                return false;
            }

            if (!m_pipeline->setFragmentShader(fsh)) {
                shutdown();
                return false;
            }

            if (!m_pipeline->init()) {
                shutdown();
                return false;
            }

            u32 swapChainImageCount = m_swapChain->getImages().size();
            m_vertices.reserve(swapChainImageCount);
            m_frameVertices.reserve(swapChainImageCount);
            m_frameUniforms.reserve(swapChainImageCount);
            m_frameDescriptorSets.reserve(swapChainImageCount);
            for (u32 i = 0;i < swapChainImageCount;i++) {
                m_vertices.emplace(m_maxLines * 2);
                vulkan::Vertices* v = m_vboFactory->allocate(&m_vfmt, maxLines * 2);
                if (!v) {
                    shutdown();
                    return false;
                }

                m_frameVertices.push(v);

                vulkan::UniformObject* u = m_uboFactory->allocate(&m_ufmt);
                if (!u) {
                    shutdown();
                    return false;
                }

                vulkan::DescriptorSet* s = m_dsFactory->allocate(m_pipeline);
                if (!s) {
                    shutdown();
                    return false;
                }

                s->add(u, 0);
                s->update();

                m_frameUniforms.push(u);
                m_frameDescriptorSets.push(s);
            }
            
            m_btnDown = false;
            m_moveSpeed = 10.0f;
            m_moveDamping = 0.93f;
            m_moveAccel = 20.5f;
            m_moveVelocity = { 0.0f, 0.0f, 0.0f };
            m_dt.start();
            auto e = m_pipeline->getSwapChain()->getExtent();
            m_projection = mat4f::Perspective(::utils::radians(70.0f), f32(e.width) / f32(e.height), 0.1f, 100.0f);
            m_view = mat4f::LookAt(
                vec3f(10.0f, 10.0f, 10.0f),
                vec3f(0.0f, 0.0f, 0.0f),
                vec3f(0.0f, 1.0f, 0.0f)
            );
            m_manualProjection = false;
            m_manualView = false;
            for (u32 i = 0;i < 256;i++) m_keyDown[i] = false;

            return true;
        }

        void SimpleDebugDraw::shutdown() {
            m_frameVertices.each([](vulkan::Vertices* v) { v->free(); });
            m_frameDescriptorSets.each([](vulkan::DescriptorSet* s) { s->free(); });
            m_frameUniforms.each([](vulkan::UniformObject* u) { u->free(); });

            if (m_pipeline) {
                m_pipeline->shutdown();
                delete m_pipeline;
            }

            m_swapChain = nullptr;
            m_renderPass = nullptr;
            m_vboFactory = nullptr;
            m_uboFactory = nullptr;
            m_pipeline = nullptr;
            m_vertices.clear(true);
            m_frameVertices.clear(true);
            m_frameDescriptorSets.clear(true);
            m_frameUniforms.clear(true);
        }

        void SimpleDebugDraw::setProjection(const mat4f& proj) {
            m_projection = proj;
            m_manualProjection = true;
        }

        void SimpleDebugDraw::setView(const mat4f& view) {
            m_view = view;
            m_manualView = true;
        }

        const mat4f& SimpleDebugDraw::getProjection() const {
            return m_projection;
        }

        const mat4f& SimpleDebugDraw::getView() const {
            return m_view;
        }

        void SimpleDebugDraw::line(const vec3f& a, const vec3f& b, const vec4f& color) {
            m_vertices[m_currentFrameIdx].push({ a, color });
            m_vertices[m_currentFrameIdx].push({ b, color });
        }

        void SimpleDebugDraw::begin(u32 currentSwapChainImageIndex) {
            f32 dt = m_dt;
            m_dt.reset();
            m_dt.start();

            if (!m_manualView) {
                if (m_keyDown[::utils::KEY_W]) m_moveVelocity.z += m_moveAccel * m_moveSpeed * dt;
                if (m_keyDown[::utils::KEY_S]) m_moveVelocity.z -= m_moveAccel * m_moveSpeed * dt;
                if (m_keyDown[::utils::KEY_A]) m_moveVelocity.x += m_moveAccel * m_moveSpeed * dt;
                if (m_keyDown[::utils::KEY_D]) m_moveVelocity.x -= m_moveAccel * m_moveSpeed * dt;

                m_moveVelocity.x = ::utils::clamp(m_moveVelocity.x, -m_moveSpeed, m_moveSpeed);
                m_moveVelocity.y = ::utils::clamp(m_moveVelocity.y, -m_moveSpeed, m_moveSpeed);
                m_moveVelocity.z = ::utils::clamp(m_moveVelocity.z, -m_moveSpeed, m_moveSpeed);

                m_view *= mat4f::Translation(m_moveVelocity * dt);
            }

            m_moveVelocity *= m_moveDamping;

            m_currentFrameIdx = currentSwapChainImageIndex;
            m_vertices[m_currentFrameIdx].clear(false);
        }

        void SimpleDebugDraw::end(vulkan::CommandBuffer* cb) {
            Array<vertex>& verts = m_vertices[m_currentFrameIdx];
            if (verts.size() == 0) return;

            if (!m_manualProjection) {
                auto e = m_pipeline->getSwapChain()->getExtent();
                m_projection = mat4f::Perspective(::utils::radians(70.0f), f32(e.width) / f32(e.height), 0.1f, 100.0f);
            }

            m_uniforms.viewProj = m_view * m_projection;

            vulkan::UniformObject* u = m_frameUniforms[m_currentFrameIdx];
            u->set(m_uniforms);
            u->getBuffer()->submitUpdates(cb);

            vulkan::Vertices* v = m_frameVertices[m_currentFrameIdx];
            if (v->beginUpdate()) {
                v->write(verts.data(), 0, verts.size());
                v->commitUpdate();
            }
        }

        void SimpleDebugDraw::draw(vulkan::CommandBuffer* cb) {
            Array<vertex>& verts = m_vertices[m_currentFrameIdx];
            if (verts.size() == 0) return;

            cb->bindPipeline(m_pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);
            auto e = m_pipeline->getSwapChain()->getExtent();
            cb->setViewport(0, e.height, e.width, -f32(e.height), 0, 1);
            cb->setScissor(0, 0, e.width, e.height);

            vulkan::DescriptorSet* d = m_frameDescriptorSets[m_currentFrameIdx];
            vulkan::UniformObject* u = m_frameUniforms[m_currentFrameIdx];
            vulkan::Vertices* v = m_frameVertices[m_currentFrameIdx];

            cb->bindDescriptorSet(d, VK_PIPELINE_BIND_POINT_GRAPHICS);
            cb->bindVertexBuffer(v->getBuffer());
            cb->draw(verts.size(), v->getOffset());
        }

        void SimpleDebugDraw::onMouseMove(i32 x, i32 y) {
            constexpr f32 rotSpeed = 50.0f;
            vec2f cur = { f32(x), f32(y) };
            
            if (m_btnDown && !m_manualView) {
                auto e = m_pipeline->getSwapChain()->getExtent();

                vec2f delta = {
                    cur.x - m_cursor.x,
                    cur.y - m_cursor.y
                };

                vec2f scale = {
                    fabsf(delta.x) / f32(e.width),
                    fabsf(delta.y) / f32(e.height)
                };

                vec3f xAxis = vec3f(1.0f, 0.0f, 0.0f);
                vec3f yAxis = m_view.basis() * vec3f(0.0f, 1.0f, 0.0f);

                if (delta.x < 0.0f) {
                    m_view *= mat4f::Rotation(yAxis, ::utils::radians(rotSpeed * scale.x));
                } else if (delta.x > 0.0f) {
                    m_view *= mat4f::Rotation(yAxis, ::utils::radians(-rotSpeed * scale.x));
                }

                if (delta.y < 0.0f) {
                    m_view *= mat4f::Rotation(xAxis, ::utils::radians(rotSpeed * scale.y));
                } else if (delta.y > 0.0f) {
                    m_view *= mat4f::Rotation(xAxis, ::utils::radians(-rotSpeed * scale.y));
                }
            }

            m_cursor = cur;
        }

        void SimpleDebugDraw::onScroll(f32 delta) {
            if (m_manualView) return;

            m_moveSpeed += delta;
            m_moveSpeed = ::utils::clamp(m_moveSpeed, 0.1f, 100.0f);
        }

        void SimpleDebugDraw::onMouseDown(::utils::MouseButton buttonIdx) {
            if (buttonIdx != ::utils::LEFT_BUTTON) return;
            m_btnDown = true;
        }

        void SimpleDebugDraw::onMouseUp(::utils::MouseButton buttonIdx) {
            if (buttonIdx != ::utils::LEFT_BUTTON) return;
            m_btnDown = false;
        }

        void SimpleDebugDraw::onKeyDown(::utils::KeyboardKey key) {
            m_keyDown[key] = true;
        }

        void SimpleDebugDraw::onKeyUp(::utils::KeyboardKey key) {
            m_keyDown[key] = false;
        }
    };
};