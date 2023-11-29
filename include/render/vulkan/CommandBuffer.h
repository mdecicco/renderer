#pragma once
#include <render/types.h>

#include <vulkan/vulkan.h>
namespace render {
    namespace vulkan {
        class LogicalDevice;
        class CommandPool;
        class Pipeline;
        class VertexBuffer;
        class Vertices;

        class CommandBuffer {
            public:
                VkCommandBuffer get() const;
                CommandPool* getPool() const;

                bool begin(VkCommandBufferUsageFlagBits flags = VkCommandBufferUsageFlagBits(0));
                bool end();
                bool reset();

                void beginRenderPass(Pipeline* pipeline, const VkClearValue& clearColor, u32 imageIdx);
                void endRenderPass();
                void bindPipeline(Pipeline* pipeline, VkPipelineBindPoint bindPoint);
                void bindVertexBuffer(VertexBuffer* vbo);
                void setViewport(f32 x, f32 y, f32 width, f32 height, f32 minZ, f32 maxZ);
                void setScissor(i32 x, i32 y, u32 width, u32 height);
                void draw(Vertices* vertices);
                void draw(u32 vertexCount, u32 firstVertex = 0, u32 instanceCount = 1, u32 firstInstance = 0);

            protected:
                friend class CommandPool;
                LogicalDevice* m_device;
                CommandPool* m_pool;
                VkCommandBuffer m_buffer;
                bool m_isRecording;

                CommandBuffer();
                ~CommandBuffer();
        };
    };
};