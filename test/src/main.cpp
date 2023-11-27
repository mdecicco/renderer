#include <exception>
#include <stdio.h>

#include <render/IWithRendering.h>
#include <render/vulkan/Pipeline.h>
#include <render/vulkan/CommandPool.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/Queue.h>
#include <render/vulkan/LogicalDevice.h>

#include <utils/Allocator.hpp>
#include <utils/Singleton.hpp>
#include <utils/Window.h>

class TestApp : public render::IWithRendering {
    public:
        TestApp() {
            m_window.setPosition(500, 500);
            m_window.setSize(800, 600);
            m_window.setTitle("Vulkan Test");

            m_pipeline = nullptr;
            m_cmdPool = nullptr;
            m_cmdBuf = nullptr;
        }

        virtual ~TestApp() {
            if (m_cmdPool) delete m_cmdPool;
            m_cmdPool = nullptr;
            if (m_pipeline) delete m_pipeline;
            m_pipeline = nullptr;
        }

        bool initialize() {
            if (!m_window.setOpen(true)) return false;
            if (!initRendering(&m_window)) return false;

            m_pipeline = new render::vulkan::Pipeline(
                getShaderCompiler(),
                getLogicalDevice(),
                getSwapChain()
            );

            const char* vsh =
                "layout (location = 0) in vec3 v_pos;\n"
                "\n"
                "void main() {\n"
                "  gl_Position = vec4(v_pos, 1.0);\n"
                "}\n"
            ;
            const char* fsh =
                "layout (location = 1) out vec4 o_color;\n"
                "\n"
                "void main() {\n"
                "    o_color = vec4(1.0, 1.0, 1.0, 1.0);\n"
                "}\n"
            ;
            
            render::core::VertexFormat vfmt;
            vfmt.addAttr(render::dt_vec3f);

            m_pipeline->setVertexFormat(vfmt);

            if (!m_pipeline->setVertexShader(vsh)) return false;
            if (!m_pipeline->setFragmentShader(fsh)) return false;
            if (!m_pipeline->init()) return false;

            m_cmdPool = new render::vulkan::CommandPool(
                getLogicalDevice(),
                &getLogicalDevice()->getGraphicsQueue()->getFamily()
            );
            if (!m_cmdPool->init(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)) {
                return false;
            }

            m_cmdBuf = m_cmdPool->createBuffer(true);

            return true;
        }

        void run() {
            while (m_window.isOpen()) {
                m_window.pollEvents();
                utils::Thread::Sleep(16);
                
                begin(m_cmdBuf, m_pipeline);
                end(m_cmdBuf, m_pipeline);
            }

            waitForFrameEnd();
        }

        virtual void onLogMessage(utils::LOG_LEVEL level, const utils::String& scope, const utils::String& message) {
            utils::String msg = scope + ": " + message;
            printf("%s\n", msg.c_str());
            fflush(stdout);
        }
    
    protected:
        utils::Window m_window;
        render::vulkan::Pipeline* m_pipeline;
        render::vulkan::CommandPool* m_cmdPool;
        render::vulkan::CommandBuffer* m_cmdBuf;
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