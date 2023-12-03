#include <exception>
#include <stdio.h>

#include <render/IWithRendering.h>
#include <render/vulkan/Pipeline.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/VertexBuffer.h>
#include <render/vulkan/UniformBuffer.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/SwapChain.h>
#include <render/core/FrameContext.h>

#include <utils/Allocator.hpp>
#include <utils/Singleton.hpp>
#include <utils/Math.hpp>
#include <utils/Window.h>
#include <utils/Timer.h>

struct vertex {
    utils::vec3f pos;
    utils::vec3f color;
};

utils::vec3f hsv2rgb(const utils::vec3f& in)
{
    double      hh, p, q, t, ff;
    long        i;
    utils::vec3f         out;

    if(in.y <= 0.0) {       // < is bogus, just shuts up warnings
        out.x = in.z;
        out.y = in.z;
        out.z = in.z;
        return out;
    }
    hh = in.x;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.z * (1.0 - in.y);
    q = in.z * (1.0 - (in.y * ff));
    t = in.z * (1.0 - (in.y * (1.0 - ff)));

    switch(i) {
    case 0:
        out.x = in.z;
        out.y = t;
        out.z = p;
        break;
    case 1:
        out.x = q;
        out.y = in.z;
        out.z = p;
        break;
    case 2:
        out.x = p;
        out.y = in.z;
        out.z = t;
        break;

    case 3:
        out.x = p;
        out.y = q;
        out.z = in.z;
        break;
    case 4:
        out.x = t;
        out.y = p;
        out.z = in.z;
        break;
    case 5:
    default:
        out.x = in.z;
        out.y = p;
        out.z = q;
        break;
    }
    return out;     
}

class TestApp : public render::IWithRendering {
    public:
        TestApp() {
            m_window = new utils::Window();
            m_window->setPosition(500, 500);
            m_window->setSize(256, 256);
            m_window->setTitle("Vulkan API Testing");

            m_pipeline = nullptr;
        }

        virtual ~TestApp() {
            if (m_pipeline) delete m_pipeline;
            m_pipeline = nullptr;

            shutdownRendering();

            if (m_window) delete m_window;
            m_window = nullptr;
        }

        virtual bool setupInstance(render::vulkan::Instance* instance) {
            instance->enableValidation();
            instance->subscribeLogger(this);
            return true;
        }

        bool initialize() {
            if (!m_window->setOpen(true)) return false;
            if (!initRendering(m_window)) return false;

            m_pipeline = new render::vulkan::Pipeline(
                getShaderCompiler(),
                getLogicalDevice(),
                getSwapChain()
            );

            const char* vsh =
                "layout (location = 0) in vec3 v_pos;\n"
                "layout (location = 1) in vec3 v_color;\n"
                "layout (location = 0) out vec3 a_color;\n"
                "layout (binding = 0) uniform _ubo {\n"
                "    mat3 model;\n"
                "} ubo;\n"
                "\n"
                "void main() {\n"
                "  gl_Position = vec4((ubo.model * v_pos) + vec3(0, 0, 0.5), 1.0);\n"
                "  a_color = v_color;\n"
                "}\n"
            ;
            const char* fsh =
                "layout (location = 0) in vec3 a_color;\n"
                "layout (location = 0) out vec4 o_color;\n"
                "\n"
                "void main() {\n"
                "    o_color = vec4(a_color, 1.0);\n"
                "}\n"
            ;
            
            m_vfmt.addAttr(render::dt_vec3f);
            m_vfmt.addAttr(render::dt_vec3f);
            m_pipeline->setVertexFormat(&m_vfmt);

            m_ufmt.addAttr(render::dt_mat3f);
            m_pipeline->addUniformBlock(0, &m_ufmt, VK_SHADER_STAGE_VERTEX_BIT);
            
            m_pipeline->addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
            m_pipeline->addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
            m_pipeline->setPrimitiveType(render::PT_TRIANGLE_FAN);

            if (!m_pipeline->setVertexShader(vsh)) return false;
            if (!m_pipeline->setFragmentShader(fsh)) return false;
            if (!m_pipeline->init()) return false;

            return true;
        }

        void run() {
            render::vulkan::Vertices* verts = allocateVertices(&m_vfmt, 362);
            if (!verts) abort();

            if (verts->beginUpdate()) {
                verts->at<vertex>(0) = {
                    utils::vec3f(0.0f, 0.0f, 0.0f),
                    utils::vec3f(0.5f, 0.5f, 0.5f)
                };
                for (utils::u32 i = 1;i <= 361;i++) {
                    utils::f32 t = utils::radians(utils::f32(i));
                    verts->at<vertex>(i) = {
                        utils::vec3f(cosf(t) * 0.5f, sinf(t) * 0.5f, 0.0f),
                        hsv2rgb(utils::vec3f(utils::f32(i), 1.0f, 0.5f))
                    };
                }
                verts->commitUpdate();
            }

            render::vulkan::UniformObject* uniforms = allocateUniformObject(&m_ufmt, 0, m_pipeline);
            if (!uniforms) abort();

            utils::Timer tmr;
            tmr.start();

            while (m_window->isOpen()) {
                m_window->pollEvents();

                auto frame = getFrame(m_pipeline);
                frame->begin();
                auto cb = frame->getCommandBuffer();

                utils::mat4f model = utils::mat4f::Rotation(utils::vec3f(0, 0, 1), tmr);
                
                uniforms->set(model);
                uniforms->getBuffer()->submitUpdates(cb);

                cb->beginRenderPass(m_pipeline, { 0.01f, 0.01f, 0.01f, 1.0f }, frame->getSwapChainImageIndex());
                cb->bindPipeline(m_pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);

                auto e = getSwapChain()->getExtent();
                cb->setViewport(0, 0, e.width, e.height, 0, 1);
                cb->setScissor(0, 0, e.width, e.height);

                cb->bindVertexBuffer(verts->getBuffer());
                cb->bindUniformObject(uniforms, VK_PIPELINE_BIND_POINT_GRAPHICS);
                cb->draw(verts);

                cb->endRenderPass();
                frame->end();

                getLogicalDevice()->waitForIdle();
                releaseFrame(frame);

                utils::Thread::Sleep(16);
            }

            verts->free();
            uniforms->free();

            getLogicalDevice()->waitForIdle();
        }

        virtual void onLogMessage(utils::LOG_LEVEL level, const utils::String& scope, const utils::String& message) {
            propagateLog(level, scope, message);

            utils::String msg = scope + ": " + message;
            printf("%s\n", msg.c_str());
            fflush(stdout);
        }
    
    protected:
        utils::Window* m_window;
        render::vulkan::Pipeline* m_pipeline;
        render::core::DataFormat m_vfmt;
        render::core::DataFormat m_ufmt;
};

int main(int argc, char** argv) {
    utils::Mem::Create();

    {
        TestApp app;

        if (!app.initialize()) abort();
        app.run();
    }

    utils::Mem::Destroy();
    return 0;
}