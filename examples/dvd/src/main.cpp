#include <exception>
#include <stdio.h>

#include <render/IWithRendering.h>
#include <render/vulkan/GraphicsPipeline.h>
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

#define STB_IMAGE_IMPLEMENTATION
#include <render/utils/stb_image.h>

#include <utils/Allocator.hpp>
#include <utils/Singleton.hpp>
#include <utils/Math.hpp>
#include <utils/Window.h>
#include <utils/Timer.h>
#include <utils/Array.hpp>

#include "dvd.h"

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
    vec4f tint;
};

vec4f hsv(const vec4f& hsv) {
    float fC = hsv.z * hsv.y;
    float fHPrime = fmod(hsv.x / 60.0, 6);
    float fX = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
    float fM = hsv.z - fC;

    vec4f rgb;
    rgb.w = 1.0f;
    
    if(0 <= fHPrime && fHPrime < 1) {
        rgb.x = fC;
        rgb.y = fX;
        rgb.z = 0;
    } else if(1 <= fHPrime && fHPrime < 2) {
        rgb.x = fX;
        rgb.y = fC;
        rgb.z = 0;
    } else if(2 <= fHPrime && fHPrime < 3) {
        rgb.x = 0;
        rgb.y = fC;
        rgb.z = fX;
    } else if(3 <= fHPrime && fHPrime < 4) {
        rgb.x = 0;
        rgb.y = fX;
        rgb.z = fC;
    } else if(4 <= fHPrime && fHPrime < 5) {
        rgb.x = fX;
        rgb.y = 0;
        rgb.z = fC;
    } else if(5 <= fHPrime && fHPrime < 6) {
        rgb.x = fC;
        rgb.y = 0;
        rgb.z = fX;
    } else {
        rgb.x = 0;
        rgb.y = 0;
        rgb.z = 0;
    }
    
    rgb.x += fM;
    rgb.y += fM;
    rgb.z += fM;

    return rgb;
}

class Screensaver : public IWithRendering {
    public:
        Screensaver(MonitorInfo* monitor) {
            m_window = new Window();
            m_window->setPosition(monitor->position.x, monitor->position.y);
            m_window->setSize(monitor->actualDimensions.x, monitor->actualDimensions.y);
            m_window->setTitle("DVD Screensaver");

            m_pipeline = nullptr;
            m_texture = nullptr;
            m_vertices = nullptr;
            m_uniforms = nullptr;
            m_descriptor = nullptr;

            vec2f vMin, vMax;
            vMin.x = 150.0f;
            vMax.x = 400.0f;
            vMin.y = 150.0f;
            vMax.y = 400.0f;

            m_velocity = vec2f(
                random(vMin.x, vMax.x),
                random(vMin.y, vMax.y)
            );
            m_acceleration = vec2f(0.0f, 0.0f);
            m_pos = vec2f(
                random(0.0f, f32(monitor->actualDimensions.x - logo_width)),
                random(0.0f, f32(monitor->actualDimensions.y - logo_height))
            );
            m_attracting = false;
            m_tint = hsv(vec4f(random(0.0f, 360.0f), 1.0f, 1.0f, 1.0f));

            m_runTime.start();
        }

        virtual ~Screensaver() {
            if (m_vertices) m_vertices->free();
            m_vertices = nullptr;

            if (m_uniforms) m_uniforms->free();
            m_uniforms = nullptr;

            if (m_descriptor) m_descriptor->free();
            m_descriptor = nullptr;

            if (m_texture) delete m_texture;
            m_texture = nullptr;

            if (m_pipeline) delete m_pipeline;
            m_pipeline = nullptr;

            shutdownRendering();

            if (m_window) delete m_window;
            m_window = nullptr;
        }

        virtual bool setupInstance(Instance* instance) {
            instance->subscribeLogger(this);
            return true;
        }

        bool initialize() {
            if (!m_window->setOpen(true)) return false;
            m_window->setBorderEnabled(false);

            ShowCursor(FALSE);

            if (!initRendering(m_window)) return false;

            m_pipeline = new GraphicsPipeline(
                getShaderCompiler(),
                getLogicalDevice(),
                getSwapChain(),
                getRenderPass()
            );

            const char* vsh =
                "layout (location = 0) in vec3 v_pos;\n"
                "layout (location = 1) in vec2 v_tex;\n"
                "layout (binding = 0) uniform _ubo {\n"
                "    mat4 projection;\n"
                "    mat4 view;\n"
                "    mat4 viewProj;\n"
                "    mat4 model;\n"
                "    vec4 tint;\n"
                "} ubo;\n"
                "\n"
                "layout (location = 0) out vec2 a_tex;\n"
                "layout (location = 1) out vec4 a_tint;\n"
                "\n"
                "void main() {\n"
                "  gl_Position = ubo.viewProj * ubo.model * vec4(v_pos, 1.0);\n"
                "  a_tex = v_tex;\n"
                "  a_tint = ubo.tint;\n"
                "}\n"
            ;
            const char* fsh =
                "layout (location = 0) in vec2 a_tex;\n"
                "layout (location = 1) in vec4 a_tint;\n"
                "layout (binding = 1) uniform sampler2D s_tex;\n"
                "\n"
                "layout (location = 0) out vec4 o_color;\n"
                "\n"
                "void main() {\n"
                "    o_color = texture(s_tex, a_tex) * a_tint;\n"
                "}\n"
            ;
            
            m_vfmt.addAttr(&vertex::pos);
            m_vfmt.addAttr(&vertex::uv);
            m_pipeline->setVertexFormat(&m_vfmt);

            m_ufmt.addAttr(&ubo::projection);
            m_ufmt.addAttr(&ubo::view);
            m_ufmt.addAttr(&ubo::viewProj);
            m_ufmt.addAttr(&ubo::model);
            m_ufmt.addAttr(&ubo::tint);
            m_pipeline->addUniformBlock(0, &m_ufmt, VK_SHADER_STAGE_VERTEX_BIT);
            m_pipeline->addSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);
            
            m_pipeline->addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
            m_pipeline->addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
            m_pipeline->setPrimitiveType(PT_TRIANGLE_FAN);
            m_pipeline->setDepthTestEnabled(true);
            m_pipeline->setDepthCompareOp(COMPARE_OP::CO_LESS_OR_EQUAL);
            m_pipeline->setDepthWriteEnabled(true);
            m_pipeline->setColorBlendEnabled(true);
            m_pipeline->setColorBlendOp(BLEND_OP::BO_ADD);
            m_pipeline->setAlphaBlendOp(BLEND_OP::BO_ADD);
            m_pipeline->setSrcColorBlendFactor(BLEND_FACTOR::BF_SRC_ALPHA);
            m_pipeline->setDstColorBlendFactor(BLEND_FACTOR::BF_ONE_MINUS_SRC_ALPHA);
            m_pipeline->setSrcAlphaBlendFactor(BLEND_FACTOR::BF_ONE);
            m_pipeline->setDstAlphaBlendFactor(BLEND_FACTOR::BF_ZERO);

            if (!m_pipeline->setVertexShader(vsh)) return false;
            if (!m_pipeline->setFragmentShader(fsh)) return false;
            if (!m_pipeline->init()) return false;

            if (!initTexture()) return false;
            if (!initDrawData()) return false;

            return true;
        }

        bool initTexture() {
            m_texture = new Texture(getLogicalDevice());
            if (!m_texture->init(logo_width, logo_height, VK_FORMAT_R8G8B8A8_SRGB)) return false;
            if (!m_texture->initStagingBuffer()) return false;
            if (!m_texture->initSampler()) return false;

            CommandBuffer* buf = getFrameManager()->getCommandPool()->createBuffer(true);
            if (buf->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
                struct dest_pixel { u8 r, g, b, a; };
                u8* data = (u8*)m_texture->getStagingBuffer()->getPointer();
                memset(data, 0, logo_width * logo_height * 4);

                for (u32 i = 0;i < pixel_count;i++) {
                    src_pixel& src = dvd_logo[i];
                    dest_pixel* px = (dest_pixel*)(data + (src.y * (logo_width * 4)) + (src.x * 4));
                    px->r = src.r;
                    px->g = src.g;
                    px->b = src.b;
                    px->a = src.a;
                }
                
                m_texture->setLayout(buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                m_texture->flushPixels(buf);
                m_texture->setLayout(buf, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                buf->end();

                getLogicalDevice()->getGraphicsQueue()->submit(buf);
                getLogicalDevice()->getGraphicsQueue()->waitForIdle();
                m_texture->shutdownStagingBuffer();
            }

            return true;
        }

        bool initDrawData() {
            m_vertices = allocateVertices(&m_vfmt, 4);
            if (!m_vertices) abort();

            if (m_vertices->beginUpdate()) {
                vec2f dims = vec2f(m_texture->getDimensions());
                u32 v = 0;
                m_vertices->at<vertex>(v++) = { vec3f(0.0f  , 0.0f  , -0.5f), vec2f(0.0f, 0.0f) };
                m_vertices->at<vertex>(v++) = { vec3f(dims.x, 0.0f  , -0.5f), vec2f(1.0f, 0.0f) };
                m_vertices->at<vertex>(v++) = { vec3f(dims.x, dims.y, -0.5f), vec2f(1.0f, 1.0f) };
                m_vertices->at<vertex>(v++) = { vec3f(0.0f  , dims.y, -0.5f), vec2f(0.0f, 1.0f) };
                m_vertices->commitUpdate();
            }

            m_uniforms = allocateUniformObject(&m_ufmt);
            if (!m_uniforms) abort();

            m_descriptor = allocateDescriptor(m_pipeline);
            if (!m_descriptor) abort();

            m_descriptor->add(m_uniforms, 0);
            m_descriptor->add(m_texture, 1);
            m_descriptor->update();

            return true;
        }

        bool service() {
            if (!m_window->isOpen()) return false;

            m_window->pollEvents();

            f32 dt = m_frameTimer;
            m_frameTimer.reset();
            m_frameTimer.start();

            if (dt == 0.0f) dt = 1.0f / 60.0f;

            update(dt);
            draw();

            getLogicalDevice()->waitForIdle();
            return true;
        }

        void update(f32 dt) {
            vec2f screenSize;
            screenSize.x = getSwapChain()->getExtent().width;
            screenSize.y = getSwapChain()->getExtent().height;

            f32 scale = 0.0001f * screenSize.x;
            vec2f texSz = m_texture->getDimensions();
            texSz *= scale;

            f32 screenMag = screenSize.magnitude();
            f32 maxSpeed = 0.1f * screenMag;
            f32 minSpeed = 0.04f * screenMag;
            f32 maxSpeedWhenAttracting = 0.3f * screenMag;
            bool didHit = false;
            bool startedAttracting = false;
            bool canStartAttracting = !m_attracting && m_hitTimer.stopped() || m_hitTimer > 1.0f;

            vec2f attractRange = screenSize * 0.1f;

            if (isnan(m_velocity.x) || isnan(m_velocity.y) || isnan(m_pos.x) || isnan(m_pos.y)) {
                vec2f vMin, vMax;
                vMin.x = 150.0f;
                vMax.x = 400.0f;
                vMin.y = 150.0f;
                vMax.y = 400.0f;

                m_velocity = vec2f(
                    random(vMin.x, vMax.x),
                    random(vMin.y, vMax.y)
                );
                m_acceleration = vec2f(0.0f, 0.0f);
                m_pos = vec2f(
                    random(0.0f, f32(screenSize.x - logo_width)),
                    random(0.0f, f32(screenSize.y - logo_height))
                );
                m_attracting = false;
                m_tint = hsv(vec4f(random(0.0f, 360.0f), 1.0f, 1.0f, 1.0f));
                m_hitTimer.reset();
            }

            m_velocity += m_acceleration * dt;
            m_pos += m_velocity * dt;
            
            if (m_pos.x + texSz.x > screenSize.x) {
                m_pos.x = screenSize.x - texSz.x;
                m_velocity.x *= -1.0f;
                didHit = true;
            } else if (m_pos.x < 0) {
                m_pos.x = 0;
                m_velocity.x *= -1.0f;
                didHit = true;
            }

            if (m_pos.y + texSz.y > screenSize.y) {
                m_pos.y = screenSize.y - texSz.y;
                m_velocity.y *= -1.0f;
                didHit = true;
            } else if (m_pos.y < 0) {
                m_pos.y = 0;
                m_velocity.y *= -1.0f;
                didHit = true;
            }

            f32 speed = m_velocity.magnitude();
            if (!m_attracting) {
                if (speed > maxSpeed) {
                    m_velocity *= 1.0f / speed;
                    m_velocity *= maxSpeed;
                } else if (speed < minSpeed) {
                    m_velocity *= 1.0f / speed;
                    m_velocity *= minSpeed;
                }
            } else {
                if (speed > maxSpeedWhenAttracting) {
                    m_velocity *= 1.0f / speed;
                    m_velocity *= maxSpeedWhenAttracting;
                }
            }

            if (canStartAttracting && m_pos.x < attractRange.x && m_velocity.x < 0.0f) m_attracting = startedAttracting = true;
            else if (canStartAttracting && m_pos.x + texSz.x > (screenSize.x - attractRange.x) && m_velocity.x > 0.0f) m_attracting = startedAttracting = true;
            if (canStartAttracting && m_pos.y < attractRange.y && m_velocity.y < 0.0f) m_attracting = startedAttracting = true;
            else if (canStartAttracting && m_pos.y + texSz.y > (screenSize.y - attractRange.y) && m_velocity.y > 0.0f) m_attracting = startedAttracting = true;

            if (m_attracting) {
                vec2f delta = m_pos - m_attractDest;
                f32 dist = delta.magnitude();

                if (dist < 10.0f) {
                    m_attracting = false;

                    vec2f vMin, vMax;
                    if (delta.x > 0) {
                        // attracted to left
                        vMin.x = 150.0f;
                        vMax.x = 400.0f;
                    } else {
                        // attracted to right
                        vMin.x = -400.0f;
                        vMax.x = 150.0f;
                    }

                    if (delta.y > 0) {
                        // attracted to top
                        vMin.y = 150.0f;
                        vMax.y = 400.0f;
                    } else {
                        // attracted to bottom
                        vMin.y = -400.0f;
                        vMax.y = 150.0f;
                    }

                    m_velocity = vec2f(
                        random(vMin.x, vMax.x),
                        random(vMin.y, vMax.y)
                    );
                    m_acceleration = vec2f(0.0f, 0.0f);

                    m_pos = m_attractDest;
                    didHit = true;

                    m_hitTimer.reset();
                    m_hitTimer.start();
                } else {
                    if (startedAttracting) {
                        m_attractDest = vec2f(0.0f, 0.0f);
                        if (m_pos.x > screenSize.x * 0.5f) m_attractDest.x = screenSize.x - texSz.x;
                        if (m_pos.y > screenSize.y * 0.5f) m_attractDest.y = screenSize.y - texSz.y;
                    }

                    f32 wallDist = FLT_MAX;
                    f32 l = m_pos.x;
                    f32 r = screenSize.x - (m_pos.x + texSz.x);
                    f32 t = m_pos.y;
                    f32 b = screenSize.y - (m_pos.y + texSz.y);
                    if (l < wallDist) wallDist = l;
                    if (r < wallDist) wallDist = r;
                    if (t < wallDist) wallDist = t;
                    if (b < wallDist) wallDist = b;
                    wallDist = abs(wallDist);

                    f32 G = 100;
                    f32 massWall = 1000.0f;
                    f32 massLogo = 100.0f;
                    f32 fg = max(G * ((massWall * massLogo) / (wallDist * wallDist)), 2500.0f);

                    m_acceleration = -delta.normalized();
                    m_acceleration *= fg;

                    f32 damping = max(min(wallDist, 0.9f), 0.0001f);
                    m_velocity *= powf(damping, dt);
                }
            } else m_acceleration = vec2f(0.0f, 0.0f);

            if (didHit) {
                m_tint = hsv(vec4f(random(0.0f, 360.0f), 1.0f, 1.0f, 1.0f));
            }
        }

        void draw() {
            vec2f texSz = m_texture->getDimensions();
            auto screenSize = m_pipeline->getSwapChain()->getExtent();

            f32 scale = 0.0001f * f32(screenSize.width);

            auto frame = getFrame();
            frame->begin();
            auto cb = frame->getCommandBuffer();

            frame->setClearColor(0, vec4f(0.0f, 0.0f, 0.0f, 1.0f));
            frame->setClearDepthStencil(1, 1.0f, 0);

            ubo u;
            u.projection = mat4f::Orthographic(0, screenSize.width, 0, screenSize.height, -1.0f, 1.0f);
            u.view = mat4f();
            u.viewProj = u.projection;
            u.model = mat4f::Scale(scale) * mat4f::Translation(vec3f(m_pos.x, m_pos.y, 0.0f));
            u.tint = m_tint;
            m_uniforms->set(u);
            m_uniforms->getBuffer()->submitUpdates(cb);

            cb->beginRenderPass(m_pipeline, frame->getFramebuffer());

            cb->bindPipeline(m_pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);

            cb->setViewport(0, screenSize.height, screenSize.width, -f32(screenSize.height), 0, 1);
            cb->setScissor(0, 0, screenSize.width, screenSize.height);

            cb->bindVertexBuffer(m_vertices->getBuffer());
            cb->bindDescriptorSet(m_descriptor, VK_PIPELINE_BIND_POINT_GRAPHICS);
            cb->draw(m_vertices);

            cb->endRenderPass();
            frame->end();

            releaseFrame(frame);
        }

        void maybeClose() {
            if (m_runTime < 1.5f) return;
            m_window->setOpen(false);
        }
        virtual void onKeyDown  (KeyboardKey key)       { maybeClose(); }
        virtual void onKeyUp    (KeyboardKey key)       { maybeClose(); }
        virtual void onChar     (u8 code)               { maybeClose(); }
        virtual void onMouseDown(MouseButton buttonIdx) { maybeClose(); }
        virtual void onMouseUp  (MouseButton buttonIdx) { maybeClose(); }
        virtual void onMouseMove(i32 x, i32 y)          { maybeClose(); }
        virtual void onScroll   (f32 delta)             { maybeClose(); }

        virtual void onLogMessage(LOG_LEVEL level, const String& scope, const String& message) {
            propagateLog(level, scope, message);

            String msg = scope + ": " + message;
            printf("%s\n", msg.c_str());
            fflush(stdout);
        }
    
    protected:
        Window* m_window;
        GraphicsPipeline* m_pipeline;
        Texture* m_texture;
        Vertices* m_vertices;
        UniformObject* m_uniforms;
        DescriptorSet* m_descriptor;
        core::DataFormat m_vfmt;
        core::DataFormat m_ufmt;
        vec2f m_velocity;
        vec2f m_acceleration;
        vec2f m_attractDest;
        bool m_attracting;
        vec2f m_pos;
        vec4f m_tint;
        u32 m_tintIndex;
        Timer m_runTime;
        Timer m_hitTimer;
        Timer m_frameTimer;
};

int main(int argc, char** argv) {
    srand(time(nullptr));

    Mem::Create();

    {
        auto monitors = Window::GetMonitors();
        if (monitors.size() > 0) {
            ::utils::Array<Screensaver*> screens;

            for (u32 i = 0;i < monitors.size();i++) {
                screens.push(new Screensaver(&monitors[i]));
            }

            for (u32 i = 0;i < screens.size();i++) {
                if (!screens[i]->initialize()) {
                    for (u32 s = 0;s < screens.size();s++) delete screens[s];
                    Mem::Destroy();
                    return 0;
                }
            }

            bool doExit = false;
            while (!doExit) {
                for (u32 i = 0;i < screens.size();i++) {
                    doExit = !screens[i]->service();
                    if (doExit) break;
                }
            }
            
            for (u32 i = 0;i < screens.size();i++) delete screens[i];
        }
    }

    Mem::Destroy();
    return 0;
}