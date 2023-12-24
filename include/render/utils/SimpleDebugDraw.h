#pragma once
#include <render/types.h>
#include <render/utils/DebugDraw.h>
#include <render/core/DataFormat.h>

#include <utils/Input.h>
#include <utils/Timer.h>

namespace render {
    namespace vulkan {
        class SwapChain;
        class VertexBufferFactory;
        class Vertices;
        class UniformBufferFactory;
        class UniformObject;
        class DescriptorFactory;
        class DescriptorSet;
        class CommandBuffer;
        class GraphicsPipeline;
        class RenderPass;
        class ShaderCompiler;
    };

    namespace utils {
        class SimpleDebugDraw : public IDebugDrawer, public ::utils::IInputHandler {
            public:
                struct vertex {
                    vec3f position;
                    vec4f color;
                };

                struct uniforms {
                    mat4f viewProj;
                };

                SimpleDebugDraw();
                virtual ~SimpleDebugDraw();

                bool init(
                    vulkan::ShaderCompiler* compiler,
                    vulkan::SwapChain* swapChain,
                    vulkan::RenderPass* renderPass,
                    vulkan::VertexBufferFactory* vboFactory,
                    vulkan::UniformBufferFactory* uboFactory,
                    vulkan::DescriptorFactory* dsFactory,
                    u32 maxLines = 4096
                );
                void shutdown();

                void setProjection(const mat4f& proj);
                void setView(const mat4f& view);
                const mat4f& getProjection() const;
                const mat4f& getView() const;

                virtual void line(const vec3f& a, const vec3f& b, const vec4f& color = { 1.0f, 1.0f, 1.0f, 1.0f });

                void begin(u32 currentSwapChainImageIndex);
                void end(vulkan::CommandBuffer* cb);
                void draw(vulkan::CommandBuffer* cb);

                virtual void onMouseMove(i32 x, i32 y);
                virtual void onScroll(f32 delta);
                virtual void onMouseDown(::utils::MouseButton buttonIdx);
                virtual void onMouseUp(::utils::MouseButton buttonIdx);
                virtual void onKeyDown(::utils::KeyboardKey key);
                virtual void onKeyUp(::utils::KeyboardKey key);

            protected:
                core::DataFormat m_vfmt;
                core::DataFormat m_ufmt;
                u32 m_maxLines;

                vulkan::VertexBufferFactory* m_vboFactory;
                vulkan::UniformBufferFactory* m_uboFactory;
                vulkan::DescriptorFactory* m_dsFactory;
                vulkan::SwapChain* m_swapChain;
                vulkan::RenderPass* m_renderPass;
                vulkan::GraphicsPipeline* m_pipeline;
                Array<vulkan::Vertices*> m_frameVertices;
                Array<vulkan::UniformObject*> m_frameUniforms;
                Array<vulkan::DescriptorSet*> m_frameDescriptorSets;

                u32 m_currentFrameIdx;
                Array<Array<vertex>> m_vertices;
                uniforms m_uniforms;

                mat4f m_projection;
                mat4f m_view;
                bool m_manualProjection;
                bool m_manualView;

                bool m_btnDown;
                bool m_keyDown[256];
                vec2f m_cursor;
                ::utils::Timer m_dt;
                f32 m_moveSpeed;
                f32 m_moveDamping;
                f32 m_moveAccel;
                vec3f m_moveVelocity;
        };
    };
};