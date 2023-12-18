#include <exception>
#include <stdio.h>

#include <render/IWithRendering.h>
#include <render/vulkan/Pipeline.h>
#include <render/vulkan/CommandPool.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/VertexBuffer.h>
#include <render/vulkan/UniformBuffer.h>
#include <render/vulkan/DescriptorSet.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/RenderPass.h>
#include <render/vulkan/Texture.h>
#include <render/vulkan/Queue.h>
#include <render/vulkan/Framebuffer.h>
#include <render/core/FrameContext.h>
#include <render/core/FrameManager.h>
#include <render/utils/SimpleDebugDraw.h>
#include <render/utils/ImGui.h>

#include <utils/Allocator.hpp>
#include <utils/Singleton.hpp>
#include <utils/Math.hpp>
#include <utils/Window.h>
#include <utils/Timer.h>

using namespace ::utils;
using namespace render;
using namespace vulkan;

struct vertex {
    vec3f pos;
    vec2f uv;
};

struct ubo {
    mat4f projection;
    mat4f view;
    mat4f viewProj;
    mat4f model;
};

class TestApp : public IWithRendering {
    public:
        TestApp() {
            m_window = new Window();
            m_window->setPosition(500, 500);
            m_window->setSize(256, 256);
            m_window->setTitle("Vulkan API Testing");

            m_pipeline = nullptr;
            m_texture = nullptr;
        }

        virtual ~TestApp() {
            if (m_texture) delete m_texture;
            m_texture = nullptr;

            if (m_pipeline) delete m_pipeline;
            m_pipeline = nullptr;

            if (m_renderPass) delete m_renderPass;
            m_renderPass = nullptr;

            shutdownRendering();

            if (m_window) delete m_window;
            m_window = nullptr;
        }

        virtual bool setupInstance(Instance* instance) {
            instance->enableValidation();
            instance->subscribeLogger(this);
            return true;
        }

        bool initialize() {
            if (!m_window->setOpen(true)) return false;
            if (!initRendering(m_window)) return false;

            m_renderPass = new RenderPass(getSwapChain());
            if (!m_renderPass->init()) return false;

            if (!initImGui(m_renderPass)) return false;
            if (!initDebugDrawing(m_renderPass)) return false;

            m_pipeline = new Pipeline(
                getShaderCompiler(),
                getLogicalDevice(),
                getSwapChain(),
                m_renderPass
            );

            const char* vsh =
                "layout (location = 0) in vec3 v_pos;\n"
                "layout (location = 1) in vec2 v_tex;\n"
                "layout (binding = 0) uniform _ubo {\n"
                "    mat4 projection;\n"
                "    mat4 view;\n"
                "    mat4 viewProj;\n"
                "    mat4 model;\n"
                "} ubo;\n"
                "\n"
                "layout (location = 0) out vec2 a_tex;\n"
                "\n"
                "void main() {\n"
                "  gl_Position = ubo.viewProj * ubo.model * vec4(v_pos, 1.0);\n"
                "  a_tex = v_tex;\n"
                "}\n"
            ;
            const char* fsh =
                "layout (location = 0) in vec2 a_tex;\n"
                "layout (binding = 1) uniform sampler2D s_tex;\n"
                "\n"
                "layout (location = 0) out vec4 o_color;\n"
                "\n"
                "void main() {\n"
                "    o_color = texture(s_tex, a_tex);\n"
                "}\n"
            ;
            
            m_vfmt.addAttr(&vertex::pos);
            m_vfmt.addAttr(&vertex::uv);
            m_pipeline->setVertexFormat(&m_vfmt);

            m_ufmt.addAttr(&ubo::projection);
            m_ufmt.addAttr(&ubo::view);
            m_ufmt.addAttr(&ubo::viewProj);
            m_ufmt.addAttr(&ubo::model);
            m_pipeline->addUniformBlock(0, &m_ufmt, VK_SHADER_STAGE_VERTEX_BIT);
            m_pipeline->addSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);
            
            m_pipeline->addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
            m_pipeline->addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
            m_pipeline->setPrimitiveType(PT_TRIANGLE_FAN);
            m_pipeline->setDepthTestEnabled(true);
            m_pipeline->setDepthCompareOp(COMPARE_OP::CO_LESS_OR_EQUAL);
            m_pipeline->setDepthWriteEnabled(true);

            if (!m_pipeline->setVertexShader(vsh)) return false;
            if (!m_pipeline->setFragmentShader(fsh)) return false;
            if (!m_pipeline->init()) return false;

            m_texture = new Texture(getLogicalDevice());
            if (!m_texture->init(8, 8, VK_FORMAT_R8G8B8A8_SRGB)) return false;
            if (!m_texture->initStagingBuffer()) return false;
            if (!m_texture->initSampler()) return false;

            return true;
        }

        void run() {
            Vertices* verts = allocateVertices(&m_vfmt, 362);
            if (!verts) abort();

            if (verts->beginUpdate()) {
                verts->at<vertex>(0) = {
                    vec3f(0.0f, 0.0f, 0.0f),
                    vec2f(0.5f, 0.5f)
                };
                for (u32 i = 1;i <= 361;i++) {
                    f32 t = radians(f32(i));
                    verts->at<vertex>(i) = {
                        vec3f(
                            cosf(t) * 0.5f,
                            0.0f,
                            sinf(t) * 0.5f
                        ),
                        vec2f(
                            (cosf(t) * 0.5f) + 0.5f,
                            (sinf(t) * 0.5f) + 0.5f
                        )
                    };
                }
                verts->commitUpdate();
            }

            UniformObject* uniforms = allocateUniformObject(&m_ufmt);
            if (!uniforms) abort();

            DescriptorSet* set = allocateDescriptor(m_pipeline);
            if (!set) abort();

            set->add(uniforms, 0);
            set->add(m_texture, 1);
            set->update();

            Timer tmr;
            tmr.start();

            ubo u;
            
            CommandBuffer* buf = m_renderPass->getFrameManager()->getCommandPool()->createBuffer(true);
            if (buf->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
                struct pixel { u8 r, g, b, a; };
                pixel* pixels = (pixel*)m_texture->getStagingBuffer()->getPointer();

                for (u32 x = 0;x < 8;x++) {
                    for (u32 y = 0;y < 8;y++) {
                        u32 idx = x + (y * 8);
                        u8 v = idx % 2 == y % 2 ? 0 : 255;
                        pixels[idx] = {
                            v, v, v, 255
                        };
                    }
                }

                m_texture->setLayout(buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                m_texture->flushPixels(buf);
                m_texture->setLayout(buf, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                buf->end();

                getLogicalDevice()->getGraphicsQueue()->submit(buf);
                getLogicalDevice()->getGraphicsQueue()->waitForIdle();
                m_texture->shutdownStagingBuffer();
            }

            auto draw = getDebugDraw();

            while (m_window->isOpen()) {
                m_window->pollEvents();

                auto frame = m_renderPass->getFrame();
                frame->begin();
                auto cb = frame->getCommandBuffer();

                frame->setClearColor(0, vec4f(0.01f, 0.01f, 0.01f, 1.0f));
                frame->setClearDepthStencil(1, 1.0f, 0);

                draw->begin(frame->getSwapChainImageIndex());
                draw->originGrid(50, 50);
                draw->sphere(0.5f, vec3f(-1.5f, 0.0f, -0.5f));
                draw->sphere(0.5f, vec3f(-1.5f, 0.0f,  0.5f));
                draw->capsule(0.4f, 1.0f + sinf(tmr * 10.0f) * 0.25f, render::utils::Axis::Y_AXIS, mat4f::Translation(vec3f(-1.5f, 0.0f, 0.0f)));
                draw->end(cb);

                u.projection = draw->getProjection();
                u.view = draw->getView();
                u.viewProj = u.view * u.projection;
                u.model = mat4f::Rotation(vec3f(0, 1, 0), tmr) * mat4f::Translation(vec3f(0.0f, 0.1f, 0.0f));
                uniforms->set(u);
                uniforms->getBuffer()->submitUpdates(cb);

                cb->beginRenderPass(m_pipeline, frame->getFramebuffer());
                draw->draw(cb);

                cb->bindPipeline(m_pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);

                auto e = m_pipeline->getSwapChain()->getExtent();
                cb->setViewport(0, e.height, e.width, -f32(e.height), 0, 1);
                cb->setScissor(0, 0, e.width, e.height);

                cb->bindVertexBuffer(verts->getBuffer());
                cb->bindDescriptorSet(set, VK_PIPELINE_BIND_POINT_GRAPHICS);
                cb->draw(verts);

                getImGui()->begin();
                ImGui::ShowDemoWindow();
                getImGui()->end(frame);

                cb->endRenderPass();
                frame->end();


                // todo: find out why frame's command buffer is still pending if
                //       waitForIdle is not called here
                getLogicalDevice()->waitForIdle();
                m_renderPass->releaseFrame(frame);
            }

            verts->free();
            uniforms->free();
            set->free();

            getLogicalDevice()->waitForIdle();
        }

        virtual void onLogMessage(LOG_LEVEL level, const String& scope, const String& message) {
            propagateLog(level, scope, message);

            String msg = scope + ": " + message;
            printf("%s\n", msg.c_str());
            fflush(stdout);
        }
    
    protected:
        Window* m_window;
        Pipeline* m_pipeline;
        RenderPass* m_renderPass;
        Texture* m_texture;
        core::DataFormat m_vfmt;
        core::DataFormat m_ufmt;
};

int main(int argc, char** argv) {
    Mem::Create();

    {
        TestApp app;

        if (!app.initialize()) abort();
        app.run();
    }

    Mem::Destroy();
    return 0;
}