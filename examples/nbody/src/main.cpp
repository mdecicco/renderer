#include <exception>
#include <stdio.h>

#include <render/IWithRendering.h>
#include <render/vulkan/GraphicsPipeline.h>
#include <render/vulkan/ComputePipeline.h>
#include <render/vulkan/CommandPool.h>
#include <render/vulkan/CommandBuffer.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/PhysicalDevice.h>
#include <render/vulkan/VertexBuffer.h>
#include <render/vulkan/UniformBuffer.h>
#include <render/vulkan/DescriptorSet.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/SwapChain.h>
#include <render/vulkan/SwapChainSupport.h>
#include <render/vulkan/RenderPass.h>
#include <render/vulkan/Buffer.h>
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
#include <utils/Thread.hpp>

using namespace ::utils;
using namespace render;
using namespace vulkan;

#pragma pack(push, 1)

struct particle {
    alignas(16) vec3f pos;
    alignas(16) vec3f velocity;
    alignas(16) vec3f acceleration;
    alignas(4 ) f32 mass;
    alignas(4 ) f32 cellFilledFrac;
    alignas(4 ) f32 gridX;
    alignas(4 ) f32 gridY;
    alignas(4 ) f32 gridZ;
};

struct render_uniforms {
    mat4f viewProj;
    f32 minSpeed;
    f32 maxSpeed;
    f32 minNormalMass;
    f32 maxNormalMass;
    f32 minMassiveMass;
    f32 maxMassiveMass;
};

template <size_t N>
struct cell_data {
    u32 particleCount;
    u32 totalMass;
    u32 padding[2];

    // position is x, y, z
    // mass is w
    vec4f particles[N];
};

struct cell_read_data {
    u32 particleCount;
    u32 totalMass;
    u32 padding[2];
};
#pragma pack(pop, 1)

// spawn settings
constexpr f32 minNormalMass = 100.0f;
constexpr f32 maxNormalMass = 10001.0f;
constexpr f32 largeProbability = 0.0009f;
constexpr f32 minLargeMass = 10000000.0f;
constexpr f32 maxLargeMass = 1000000000.0f;
constexpr f32 minMassiveMass = 10000000000000.0f;
constexpr f32 maxMassiveMass = 10000000000001.0f;
constexpr f32 orbitalSpeedMult = 1.00000000f;
constexpr u32 minGalaxyCount = 80;
constexpr u32 maxGalaxyCount = 80;
constexpr u32 minRandomCount = 0;
constexpr u32 maxRandomCount = 0;
constexpr f32 minGalaxyRadius = 15.0f;
constexpr f32 maxGalaxyRadius = 90.0f;
constexpr f32 galaxyThicknessFactor = 0.05f;
constexpr f32 minGalaxySpeed = 0.01f;
constexpr f32 maxGalaxySpeed = 1.0f;

// simulation settings
constexpr f32 timeMultiplier = 1.8f;
constexpr f32 universeSize = 1000.0f;
constexpr f32 G = 6.6743e-11f;
constexpr u32 particleCount = 150000;
constexpr u32 divisionCount = 16;
constexpr u32 maxParticlesPerCell = 8192;

// display settings
constexpr f32 cameraSpeedMult = 0.25f;
constexpr f32 cameraBaseDistance = universeSize * 1.0f;
constexpr f32 cameraRotationSpeed = 2.4f * cameraSpeedMult;
constexpr f32 cameraVerticalOscillateSpeed = 1.3f * cameraSpeedMult;
constexpr f32 cameraVerticalOscillateRangeFactor = 1.0f;
constexpr f32 cameraInwardOscillateSpeed = 1.4f * cameraSpeedMult;
constexpr f32 cameraInwardOscillateRangeFactor = 1.0f;
constexpr f32 gridAlphaFactor = 0.5f;
constexpr bool renderGrid = true;

// driven by settings
constexpr f32 cellSize = (universeSize * 2.0f) / f32(divisionCount);
constexpr u64 gridSizeInBytes = (divisionCount * divisionCount * divisionCount) * sizeof(cell_data<maxParticlesPerCell>);
constexpr u64 gridSizeInMegabytes = gridSizeInBytes / 1024 / 1024;
constexpr u64 gridSizeInGigabytes = gridSizeInBytes / 1024 / 1024 / 1024;
constexpr u64 readGridSizeInBytes = (divisionCount * divisionCount * divisionCount) * sizeof(cell_read_data);
constexpr u64 readGridSizeInMegaBytes = readGridSizeInBytes / 1024 / 1024;


struct sim_context {
    CommandBuffer* commandBuf;
    CommandPool* commandPool;
    Buffer* particleGrid;
    Buffer* particleGridOut;
    Buffer* particlesIn;
    Buffer* particlesOut;
};

class OptimizeStep {
    public:
        OptimizeStep() {
            m_device = nullptr;
            m_ctx = nullptr;
            m_pipeline = nullptr;
            m_descriptor = nullptr;

            m_groupCountX = 0;
            m_groupCountY = 0;
            m_groupCountZ = 0;
            m_groupSizeX = 0;
            m_groupSizeY = 0;
            m_groupSizeZ = 0;
        }

        ~OptimizeStep() {
            shutdown();
        }

        bool init(IWithRendering* renderer) {
            auto& limits = m_device->getPhysicalDevice()->getProperties().limits;
            u32 maxGroupCountX = limits.maxComputeWorkGroupCount[0];
            u32 maxGroupCountY = limits.maxComputeWorkGroupCount[1];
            u32 maxGroupCountZ = limits.maxComputeWorkGroupCount[2];
            u32 maxGroupSizeX = limits.maxComputeWorkGroupSize[0];
            u32 maxGroupSizeY = limits.maxComputeWorkGroupSize[1];
            u32 maxGroupSizeZ = limits.maxComputeWorkGroupSize[2];

            m_groupSizeX = maxGroupSizeX;
            m_groupSizeY = 1;
            m_groupSizeZ = 1;

            u32 particlesPerGroup = m_groupSizeX * m_groupSizeY * m_groupSizeZ;

            m_groupCountX = particleCount / particlesPerGroup;
            m_groupCountY = 1;
            m_groupCountZ = 1;

            if (m_groupCountX * particlesPerGroup < particleCount) {
                m_groupCountX++;
            }

            char csh[8192] = { 0 };
            snprintf(csh, 8192,
                "layout (local_size_x = %d, local_size_y = %d, local_size_z = %d) in;\n"
                "\n"
                "struct particle {\n"
                "    vec3 pos;\n"
                "    vec3 velocity;\n"
                "    vec3 acceleration;\n"
                "    float mass;\n"
                "    float cellFilledFrac;\n"
                "    float gridX;\n"
                "    float gridY;\n"
                "    float gridZ;\n"
                "};\n"
                "\n"
                "struct grid_cell {\n"
                "    uint particleCount;\n"
                "    uint totalMass;\n"
                "    vec4 particles[%d]; // mass stored as w component\n"
                "};\n"
                "\n"
                "struct read_grid_cell {\n"
                "    uint particleCount;\n"
                "    uint totalMass;\n"
                "};\n"
                "\n"
                "layout(std140, binding = 0) readonly buffer b_in {\n"
                "   particle particlesIn[];\n"
                "};\n"
                "\n"
                "layout(std140, binding = 1) buffer b_out {\n"
                "   grid_cell gridCells[%d];\n"
                "};\n"
                "\n"
                "layout(std140, binding = 2) buffer b_read_out {\n"
                "   read_grid_cell readGridCells[%d];\n"
                "};\n"
                "\n"
                "void main() {\n"
                "    uint particleIndex = gl_GlobalInvocationID.x;\n"
                "    if (particleIndex >= %d) return;\n"
                "    particle self = particlesIn[particleIndex];\n"
                "    vec3 adjPos = self.pos + vec3(%f, %f, %f);\n"
                "    ivec3 gridCoord = ivec3(floor(adjPos * %f));\n"
                "    int gridIndex = (gridCoord.z * %d) + (gridCoord.y * %d) + gridCoord.x;\n"
                "    if (gridIndex < 0 || gridIndex >= %d) return;\n"
                "    uint writeIndex = atomicAdd(gridCells[gridIndex].particleCount, 1);\n"
                "    atomicAdd(readGridCells[gridIndex].particleCount, 1);\n"
                "    if (writeIndex >= %d) {\n"
                "        gridCells[gridIndex].particleCount = %d;\n"
                "        readGridCells[gridIndex].particleCount = %d;\n"
                "        // still add the mass\n"
                "        atomicAdd(gridCells[gridIndex].totalMass, uint(self.mass));\n"
                "        atomicAdd(readGridCells[gridIndex].totalMass, uint(self.mass));\n"
                "        return;\n"
                "    }\n"
                "    gridCells[gridIndex].particles[writeIndex] = vec4(self.pos, self.mass);\n"
                "    atomicAdd(gridCells[gridIndex].totalMass, uint(self.mass));\n"
                "    atomicAdd(readGridCells[gridIndex].totalMass, uint(self.mass));\n"
                "}\n",
                m_groupSizeX, m_groupSizeY, m_groupSizeZ,
                maxParticlesPerCell,
                divisionCount * divisionCount * divisionCount,
                divisionCount * divisionCount * divisionCount,
                particleCount,
                universeSize, universeSize, universeSize,
                1.0f / cellSize,
                divisionCount * divisionCount, divisionCount,
                divisionCount * divisionCount * divisionCount,
                maxParticlesPerCell, maxParticlesPerCell,
                maxParticlesPerCell
            );

            m_pipeline = new ComputePipeline(renderer->getShaderCompiler(), m_device);
            m_pipeline->subscribeLogger(renderer);

            if (!m_pipeline->setComputeShader(csh)) return false;
            m_pipeline->addStorageBuffer(0);
            m_pipeline->addStorageBuffer(1);
            m_pipeline->addStorageBuffer(2);

            if (!m_pipeline->init()) return false;

            m_descriptor = renderer->allocateDescriptor(m_pipeline);
            if (!m_descriptor) return false;
            m_descriptor->add(m_ctx->particlesOut, 0);
            m_descriptor->add(m_ctx->particleGrid, 1);
            m_descriptor->add(m_ctx->particleGridOut, 2);
            m_descriptor->update();

            return true;
        }

        void shutdown() {
            if (m_pipeline) delete m_pipeline;
            m_pipeline = nullptr;

            if (m_descriptor) m_descriptor->free();
            m_descriptor = nullptr;
        }

        void execute() {
            if (divisionCount == 1) return;
            if (!m_ctx->commandBuf->reset() || !m_ctx->commandBuf->begin()) return;

            vkCmdFillBuffer(m_ctx->commandBuf->get(), m_ctx->particleGrid->get(), 0, gridSizeInBytes, 0);
            vkCmdFillBuffer(m_ctx->commandBuf->get(), m_ctx->particleGridOut->get(), 0, readGridSizeInBytes, 0);

            m_ctx->commandBuf->bindPipeline(m_pipeline, VK_PIPELINE_BIND_POINT_COMPUTE);
            m_ctx->commandBuf->bindDescriptorSet(m_descriptor, VK_PIPELINE_BIND_POINT_COMPUTE);
            vkCmdDispatch(m_ctx->commandBuf->get(), m_groupCountX, m_groupCountY, m_groupCountZ);

            if (!m_ctx->commandBuf->end()) return;
            if (!m_device->getComputeQueue()->submit(m_ctx->commandBuf)) return;
            
            m_device->getComputeQueue()->waitForIdle();
        }

        sim_context* m_ctx;
        LogicalDevice* m_device;
        ComputePipeline* m_pipeline;
        DescriptorSet* m_descriptor;
        u32 m_groupCountX;
        u32 m_groupCountY;
        u32 m_groupCountZ;
        u32 m_groupSizeX;
        u32 m_groupSizeY;
        u32 m_groupSizeZ;
};

class SimulateStep {
    public:
        struct uniforms {
            alignas(4) f32 deltaTime;
            alignas(4) f32 G;
            alignas(4) u32 particleCount;
        };

        SimulateStep() {
            m_device = nullptr;
            m_ctx = nullptr;
            m_pipeline = nullptr;
            m_descriptor = nullptr;
            m_uniforms = nullptr;

            m_groupCountX = 0;
            m_groupCountY = 0;
            m_groupCountZ = 0;
            m_groupSizeX = 0;
            m_groupSizeY = 0;
            m_groupSizeZ = 0;
        }

        ~SimulateStep() {
            shutdown();
        }

        bool init(IWithRendering* renderer) {
            auto& limits = m_device->getPhysicalDevice()->getProperties().limits;
            u32 maxGroupCountX = limits.maxComputeWorkGroupCount[0];
            u32 maxGroupCountY = limits.maxComputeWorkGroupCount[1];
            u32 maxGroupCountZ = limits.maxComputeWorkGroupCount[2];
            u32 maxGroupSizeX = limits.maxComputeWorkGroupSize[0];
            u32 maxGroupSizeY = limits.maxComputeWorkGroupSize[1];
            u32 maxGroupSizeZ = limits.maxComputeWorkGroupSize[2];

            m_groupSizeX = 10;
            m_groupSizeY = 10;
            m_groupSizeZ = 10;

            u32 particlesPerGroup = m_groupSizeX * m_groupSizeY * m_groupSizeZ;
            u32 groupCount = particleCount / particlesPerGroup;
            if (groupCount * particlesPerGroup < particleCount) groupCount++;
            u32 gcDims[3] = { 1, 1, 1 };
            for (u32 i = 0;i < groupCount;i++) gcDims[i % 3]++;
            m_groupCountX = gcDims[0];
            m_groupCountY = gcDims[1];
            m_groupCountZ = gcDims[2];

            m_fmt.addAttr(&uniforms::deltaTime);
            m_fmt.addAttr(&uniforms::G);
            m_fmt.addAttr(&uniforms::particleCount);

            m_pipeline = new ComputePipeline(renderer->getShaderCompiler(), m_device);
            m_pipeline->subscribeLogger(renderer);

            if (divisionCount == 1) {
                if (!initBruteForceShader()) return false;
            } else {
                if (!initOptimizedShader()) return false;
            }

            m_pipeline->addUniformBlock(0);
            m_pipeline->addStorageBuffer(1);
            m_pipeline->addStorageBuffer(2);
            m_pipeline->addStorageBuffer(3);

            if (!m_pipeline->init()) return false;

            m_uniforms = renderer->allocateUniformObject(&m_fmt);
            if (!m_uniforms) return false;

            m_descriptor = renderer->allocateDescriptor(m_pipeline);
            if (!m_descriptor) return false;
            m_descriptor->add(m_uniforms, 0);
            m_descriptor->add(m_ctx->particlesIn, 1);
            m_descriptor->add(m_ctx->particleGrid, 2);
            m_descriptor->add(m_ctx->particlesOut, 3);
            m_descriptor->update();

            if (!initParticles()) return false;

            return true;
        }

        bool initBruteForceShader() {
            char csh[8192] = { 0 };
            snprintf(csh, 8192,
                "layout (local_size_x = %d, local_size_y = %d, local_size_z = %d) in;\n"
                "\n"
                "struct particle {\n"
                "    vec3 pos;\n"
                "    vec3 velocity;\n"
                "    vec3 acceleration;\n"
                "    float mass;\n"
                "    float cellFilledFrac;\n"
                "    float gridX;\n"
                "    float gridY;\n"
                "    float gridZ;\n"
                "};\n"
                "\n"
                "struct grid_cell {\n"
                "    uint particleCount;\n"
                "    uint totalMass;\n"
                "    vec4 particles[%d];\n"
                "};\n"
                "\n"
                "layout (binding = 0) uniform _ubo {\n"
                "    float deltaTime;\n"
                "    float G;\n"
                "    uint particleCount;\n"
                "} ubo;\n"
                "\n"
                "layout(std140, binding = 1) readonly buffer b_in {\n"
                "   particle particlesIn[];\n"
                "};\n"
                "\n"
                "layout(std140, binding = 2) readonly buffer b_cells {\n"
                "   grid_cell gridCells[%d];\n"
                "};\n"
                "\n"
                "layout(std140, binding = 3) buffer b_out {\n"
                "   particle particlesOut[];\n"
                "};\n"
                "\n"
                "void main() {\n"
                "    uint WorkGroupIndex = (gl_WorkGroupID.y * gl_NumWorkGroups.x) + (gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y) + gl_WorkGroupID.x;\n"
                "    uint selfIndex = (WorkGroupIndex * gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z) + gl_LocalInvocationIndex;\n"
                "    if (selfIndex >= ubo.particleCount) return;\n"
                "    particle self = particlesIn[selfIndex];\n"
                "    vec3 totalForce = vec3(0.0, 0.0, 0.0);\n"
                "    for (uint i = 0;i < ubo.particleCount;i++) {\n"
                "        if (i == selfIndex) continue;\n"
                "        particle p = particlesIn[i];\n"
                "        vec3 dp = self.pos - p.pos;\n"
                "        float dist = length(dp);\n"
                "        vec3 dir = dp * -(1.0 / dist);\n"
                "        float force = ubo.G * ((self.mass * p.mass) / (dist * dist));\n"
                "        totalForce += dir * force;\n"
                "    }\n"
                "    particlesOut[selfIndex].acceleration = totalForce * (1.0 / self.mass);\n"
                "    // particlesOut[selfIndex].velocity *= pow(0.999, ubo.deltaTime);\n"
                "    particlesOut[selfIndex].velocity = self.velocity + particlesOut[selfIndex].acceleration * ubo.deltaTime;\n"
                "    particlesOut[selfIndex].pos = self.pos + particlesOut[selfIndex].velocity * ubo.deltaTime;\n"
                "}\n",
                m_groupSizeX, m_groupSizeY, m_groupSizeZ,
                maxParticlesPerCell,
                divisionCount * divisionCount * divisionCount
            );

            if (!m_pipeline->setComputeShader(csh)) return false;

            return true;
        }

        bool initOptimizedShader() {
            char csh[8192] = { 0 };
            snprintf(csh, 8192,
                "layout (local_size_x = %d, local_size_y = %d, local_size_z = %d) in;\n"
                "\n"
                "struct particle {\n"
                "    vec3 pos;\n"
                "    vec3 velocity;\n"
                "    vec3 acceleration;\n"
                "    float mass;\n"
                "    float cellFilledFrac;\n"
                "    float gridX;\n"
                "    float gridY;\n"
                "    float gridZ;\n"
                "};\n"
                "\n"
                "struct grid_cell {\n"
                "    uint particleCount;\n"
                "    uint totalMass;\n"
                "    vec4 particles[%d];\n"
                "};\n"
                "\n"
                "layout (binding = 0) uniform _ubo {\n"
                "    float deltaTime;\n"
                "    float G;\n"
                "    uint particleCount;\n"
                "} ubo;\n"
                "\n"
                "layout(std140, binding = 1) readonly buffer b_in {\n"
                "   particle particlesIn[];\n"
                "};\n"
                "\n"
                "layout(std140, binding = 2) readonly buffer b_cells {\n"
                "   grid_cell gridCells[%d];\n"
                "};\n"
                "\n"
                "layout(std140, binding = 3) buffer b_out {\n"
                "   particle particlesOut[];\n"
                "};\n"
                "\n"
                "int getGridIndex(int gx, int gy, int gz) {\n"
                "    return (gz * %d) + (gy * %d) + gx;\n"
                "}\n"
                "\n"
                "bool isGridIndexValid(int idx) {\n"
                "    return idx >= 0 && idx < %d;\n"
                "}\n"
                "\n"
                "vec3 processCell(ivec3 gridCoord, vec3 selfPos, float selfMass) {\n"
                "    int cellIndex = getGridIndex(gridCoord.x, gridCoord.y, gridCoord.z);\n"
                "    uint particleCount = gridCells[cellIndex].particleCount;\n"
                "    vec3 outForce = vec3(0.0, 0.0, 0.0);\n"
                "    for (uint i = 0;i < particleCount;i++) {\n"
                "        vec4 p = gridCells[cellIndex].particles[i];\n"
                "        vec3 pPos = p.xyz;\n"
                "        if (selfPos == pPos) continue;\n"
                "        float pMass = p.w;\n"
                "\n"
                "        vec3 dp = selfPos - pPos;\n"
                "        float dist = length(dp);\n"
                "        vec3 dir = dp * -(1.0 / dist);\n"
                "        float force = ubo.G * ((selfMass * pMass) / (dist * dist));\n"
                "        outForce += dir * min(force, 10000000000000.0);\n"
                "    }\n"
                "\n"
                "    return outForce;\n"
                "}\n"
                "\n"
                "void main() {\n"
                "    uint WorkGroupIndex = (gl_WorkGroupID.y * gl_NumWorkGroups.x) + (gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y) + gl_WorkGroupID.x;\n"
                "    uint selfIndex = (WorkGroupIndex * gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z) + gl_LocalInvocationIndex;\n"
                "    if (selfIndex >= ubo.particleCount) return;\n"
                "    particle self = particlesIn[selfIndex];\n"
                "    vec3 adjPos = self.pos + vec3(%f, %f, %f);\n"
                "    ivec3 gridCoord = ivec3(floor(adjPos * %f));\n"
                "\n"
                "    vec3 totalForce = vec3(0.0, 0.0, 0.0);\n"
                "    /*\n"
                "    for (int x = -1;x < 1;x++) {\n"
                "        for (int y = -1;y < 1;y++) {\n"
                "            for (int z = -1;z < 1;z++) {\n"
                "                ivec3 coord = ivec3(x, y, z) + gridCoord;\n"
                "                if (isGridIndexValid(getGridIndex(coord.x, coord.y, coord.z))) {\n"
                "                     totalForce += processCell(coord, self.pos, self.mass);\n"
                "                }\n"
                "            }\n"
                "        }\n"
                "    }\n"
                "    */\n"
                "\n"
                "     bool isInGrid = isGridIndexValid(getGridIndex(gridCoord.x, gridCoord.y, gridCoord.z));\n"
                "     for (int x = 0;x < %d;x++) {\n"
                "        for (int y = 0;y < %d;y++) {\n"
                "            for (int z = 0;z < %d;z++) {\n"
                "                ivec3 coord = ivec3(x, y, z);\n"
                "                ivec3 coordDiff = abs(coord - gridCoord);\n"
                "                if (isInGrid && coordDiff.x <= 1 && coordDiff.y <= 1 && coordDiff.z <= 1) {\n"
                "                     totalForce += processCell(coord, self.pos, self.mass);\n"
                "                } else {\n"
                "                    //continue;\n"
                "                    uint gIdx = getGridIndex(x, y, z);\n"
                "                    if (gridCells[gIdx].particleCount < 50) continue;\n"
                "                    vec3 cellCenter = vec3(\n"
                "                        ((float(x) * %f) + %f) - %f,\n"
                "                        ((float(y) * %f) + %f) - %f,\n"
                "                        ((float(z) * %f) + %f) - %f\n"
                "                    );\n"
                "                    float cellMass = float(gridCells[gIdx].totalMass);\n"
                "                    vec3 dp = self.pos - cellCenter;\n"
                "                    float dist = length(dp);\n"
                "                    vec3 dir = dp * -(1.0 / dist);\n"
                "                    float force = clamp(ubo.G * ((self.mass * cellMass) / (dist * dist)), 0.0, 10000.0);\n"
                "                    totalForce += dir * force;\n"
                "                }\n"
                "            }\n"
                "        }\n"
                "    }\n"
                "\n"
                "    particlesOut[selfIndex].acceleration = totalForce * (1.0 / self.mass);\n"
                "    // particlesOut[selfIndex].velocity *= pow(0.999, ubo.deltaTime);\n"
                "    particlesOut[selfIndex].velocity = self.velocity + particlesOut[selfIndex].acceleration * ubo.deltaTime;\n"
                "    particlesOut[selfIndex].pos = self.pos + particlesOut[selfIndex].velocity * ubo.deltaTime;\n"
                "}\n",
                m_groupSizeX, m_groupSizeY, m_groupSizeZ,
                maxParticlesPerCell,
                divisionCount * divisionCount * divisionCount,
                divisionCount * divisionCount, divisionCount,
                divisionCount * divisionCount * divisionCount,
                universeSize, universeSize, universeSize,
                1.0f / cellSize,
                divisionCount, divisionCount, divisionCount,
                cellSize, cellSize * 0.5f, universeSize,
                cellSize, cellSize * 0.5f, universeSize,
                cellSize, cellSize * 0.5f, universeSize
            );
            
            if (!m_pipeline->setComputeShader(csh)) return false;

            return true;
        }

        void shutdown() {
            if (m_pipeline) delete m_pipeline;
            m_pipeline = nullptr;

            if (m_descriptor) m_descriptor->free();
            m_descriptor = nullptr;

            if (m_uniforms) m_uniforms->free();
            m_uniforms = nullptr;
        }
        
        bool initParticles() {
            Buffer* particles = new Buffer(m_device);
            bool r = particles->init(
                particleCount * sizeof(particle),
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_SHARING_MODE_EXCLUSIVE,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            if (!r || !particles->map()) {
                delete particles;
                return false;
            }

            spawnParticles((particle*)particles->getPointer());

            if (!particles->flush()) {
                delete particles;
                return false;
            }

            if (!m_ctx->commandBuf->reset() || !m_ctx->commandBuf->begin()) {
                delete particles;
                return false;
            }
            
            VkBufferCopy cpy = {};
            cpy.dstOffset = cpy.srcOffset = 0;
            cpy.size = particleCount * sizeof(particle);

            vkCmdCopyBuffer(
                m_ctx->commandBuf->get(),
                particles->get(),
                m_ctx->particlesOut->get(),
                1, &cpy
            );

            if (!m_ctx->commandBuf->end()) {
                delete particles;
                return false;
            }

            if (!m_device->getComputeQueue()->submit(m_ctx->commandBuf)) {
                delete particles;
                return false;
            }
            
            m_device->getComputeQueue()->waitForIdle();
            delete particles;

            return true;
        }

        void spawnParticlesDebug(particle* buffer) {
            for (u32 i = 0;i < particleCount;i++) {
                particle& p = buffer[i];
                p.pos = vec3f(1.0f, 2.0f, 3.0f);
                p.velocity = vec3f(4.0f, 5.0f, 6.0f);
                p.acceleration = vec3f(7.0f, 8.0f, 9.0f);
                p.mass = 10.0f;
            }
        }

        void spawnParticlesRandom(particle* buffer) {
            for (u32 i = 0;i < particleCount;i++) {
                particle& p = buffer[i];
                p.pos = vec3f(
                    random(-universeSize, universeSize),
                    random(-universeSize, universeSize),
                    random(-universeSize, universeSize)
                );
                p.velocity = vec3f(
                    random(-universeSize, universeSize),
                    random(-universeSize, universeSize),
                    random(-universeSize, universeSize)
                ) * 0.001f;
                p.mass = random(minNormalMass, maxNormalMass);
            }
        }

        void spawnParticles(particle* buffer) {
            //
            // spawn galaxies
            //

            u32 galaxyCount = random(minGalaxyCount, maxGalaxyCount);
            u32 randomCount = random(minRandomCount, maxRandomCount);
            struct galaxy {
                vec3f center;
                vec3f normal;
                vec3f velocity;
                f32 radius;
                f32 mass;
                f32 spawnWeight;
            };
            ::utils::Array<galaxy> galaxies;
            for (u32 i = 0;i < galaxyCount;i++) {
                f32 massFac = random(0.0f, 1.0f);
                f32 mass = lerp(minMassiveMass, maxMassiveMass, massFac);
                f32 radius = lerp(minGalaxyRadius, maxGalaxyRadius, massFac);
                galaxies.push({
                    vec3f(
                        random(-universeSize + radius, universeSize - radius),
                        random(-universeSize + radius, universeSize - radius),
                        random(-universeSize + radius, universeSize - radius)
                    ),
                    vec3f(
                        random(-1.0f, 1.0f),
                        random(-1.0f, 1.0f),
                        random(-1.0f, 1.0f)
                    ).normalized(),
                    vec3f(
                        random(-1.0f, 1.0f),
                        random(-1.0f, 1.0f),
                        random(-1.0f, 1.0f)
                    ).normalized() * random(minGalaxySpeed, maxGalaxySpeed),
                    radius,
                    mass,
                    0.0f
                });
            }

            if (galaxyCount == 1) {
                galaxies[0].center = vec3f(0.0f, 0.0f, 0.0f);
                galaxies[0].velocity = vec3f(0.0f, 0.0f, 0.0f);
                galaxies[0].normal = vec3f(0.25f, 1.0f, 0.45f).normalized();
            }

            f32 totalRadius = 0.0f;
            for (u32 i = 0;i < galaxyCount;i++) totalRadius += galaxies[i].radius;
            for (u32 i = 0;i < galaxyCount;i++) galaxies[i].spawnWeight = galaxies[i].radius / totalRadius;

            //
            // spawn particles
            //

            i32 curParticleIdx = 0;

            // spawn galactic centers
            for (u32 i = 0;i < galaxyCount && curParticleIdx < particleCount;i++) {
                particle& p = buffer[curParticleIdx++];

                p.pos = galaxies[i].center;
                p.velocity = galaxies[i].velocity;
                p.mass = galaxies[i].mass;
            }

            // spawn random
            for (u32 i = 0;i < randomCount && curParticleIdx < particleCount;i++) {
                particle& p = buffer[curParticleIdx++];

                p.pos = vec3f(
                    random(-universeSize, universeSize),
                    random(-universeSize, universeSize),
                    random(-universeSize, universeSize)
                );
                p.velocity = vec3f();
                p.mass = random(minNormalMass, maxMassiveMass);
            }

            // spawn stars
            u32 particlesRemainingBeforeGalaxies = particleCount - curParticleIdx;
            for (u32 i = 0;i < galaxyCount && curParticleIdx < particleCount;i++) {
                galaxy& g = galaxies[i];
                u32 count = u32(f32(particlesRemainingBeforeGalaxies) * g.spawnWeight);
                for (u32 j = 0;j < count && curParticleIdx < particleCount;j++) {
                    particle& p = buffer[curParticleIdx++];
                    
                    f32 gThickness = g.radius * galaxyThicknessFactor;
                    f32 orbitalDist = powf(random(0.2f, 1.0f), 2.0f) * g.radius;

                    vec3f offset = vec3f(
                        random(-1.0f, 1.0f),
                        random(-1.0f, 1.0f),
                        random(-1.0f, 1.0f)
                    ).cross(g.normal).normalized() * orbitalDist;

                    p.pos = (g.center + offset) + (g.normal * random(-gThickness, gThickness));

                    offset = p.pos - g.center;
                    orbitalDist = offset.magnitude();
                    offset *= 1.0f / orbitalDist;

                    f32 orbitalSpeed = sqrtf((G * g.mass * orbitalSpeedMult) / orbitalDist);
                    p.velocity = offset.cross(g.normal).normalized() * orbitalSpeed;
                    p.velocity += g.velocity;

                    if (random(0.0f, 1.0f) <= largeProbability) {
                        p.mass = random(minLargeMass, maxLargeMass);
                    } else {
                        p.mass = random(minNormalMass, maxNormalMass);
                    }
                }
            }

            // just make the rest random
            while (curParticleIdx < particleCount) {
                particle& p = buffer[curParticleIdx++];
                p.pos = vec3f(
                    random(-universeSize, universeSize),
                    random(-universeSize, universeSize),
                    random(-universeSize, universeSize)
                );
                p.velocity = vec3f();
                p.mass = random(minNormalMass, maxMassiveMass);
            }
        }

        void updateInputBuffer() {
            VkBufferCopy cpy = {};
            cpy.dstOffset = cpy.srcOffset = 0;
            cpy.size = particleCount * sizeof(particle);

            vkCmdCopyBuffer(
                m_ctx->commandBuf->get(),
                m_ctx->particlesOut->get(),
                m_ctx->particlesIn->get(),
                1, &cpy
            );
        }

        void updateComputeUniforms(f32 dt) {
            m_uniforms->set<uniforms>({
                dt,
                G,
                particleCount
            });
            m_uniforms->getBuffer()->submitUpdates(m_ctx->commandBuf);
        }

        void execute(f32 dt) {
            if (!m_ctx->commandBuf->reset() || !m_ctx->commandBuf->begin()) return;

            updateInputBuffer();

            updateComputeUniforms(dt);

            m_ctx->commandBuf->bindPipeline(m_pipeline, VK_PIPELINE_BIND_POINT_COMPUTE);
            m_ctx->commandBuf->bindDescriptorSet(m_descriptor, VK_PIPELINE_BIND_POINT_COMPUTE);
            vkCmdDispatch(m_ctx->commandBuf->get(), m_groupCountX, m_groupCountY, m_groupCountZ);

            if (!m_ctx->commandBuf->end()) return;
            if (!m_device->getComputeQueue()->submit(m_ctx->commandBuf)) return;
        }

        sim_context* m_ctx;
        LogicalDevice* m_device;
        ComputePipeline* m_pipeline;
        core::DataFormat m_fmt;
        DescriptorSet* m_descriptor;
        UniformObject* m_uniforms;
        u32 m_groupCountX;
        u32 m_groupCountY;
        u32 m_groupCountZ;
        u32 m_groupSizeX;
        u32 m_groupSizeY;
        u32 m_groupSizeZ;
};

class Screensaver : public IWithRendering {
    public:
        Screensaver(MonitorInfo* monitor) {
            m_window = new Window();
            m_window->setPosition(monitor->position.x, monitor->position.y);
            m_window->setSize(monitor->actualDimensions.x, monitor->actualDimensions.y);
            m_window->setTitle("Gravity Screensaver");
            m_window->setBorderEnabled(false);

            m_simCtx.commandBuf = nullptr;
            m_simCtx.commandPool = nullptr;
            m_simCtx.particlesOut = nullptr;
            m_simCtx.particlesIn = nullptr;
            m_simCtx.particleGrid = nullptr;
            m_simCtx.particleGridOut = nullptr;
            m_gfxDescriptor = nullptr;
            m_gfxUniforms = nullptr;
            m_pipeline = nullptr;

            m_runTime.start();
            m_resetTimer.start();
        }

        virtual ~Screensaver() {
            m_optStep.shutdown();
            m_simStep.shutdown();

            if (m_simCtx.commandBuf) m_simCtx.commandPool->freeBuffer(m_simCtx.commandBuf);
            m_simCtx.commandBuf = nullptr;

            if (m_simCtx.commandPool) delete m_simCtx.commandPool;
            m_simCtx.commandPool = nullptr;
            
            if (m_simCtx.particlesOut) delete m_simCtx.particlesOut;
            m_simCtx.particlesOut = nullptr;

            if (m_simCtx.particlesIn) delete m_simCtx.particlesIn;
            m_simCtx.particlesIn = nullptr;

            if (m_simCtx.particleGrid) delete m_simCtx.particleGrid;
            m_simCtx.particleGrid = nullptr;

            if (m_simCtx.particleGridOut) delete m_simCtx.particleGridOut;
            m_simCtx.particleGridOut = nullptr;

            if (m_gfxDescriptor) m_gfxDescriptor->free();
            m_gfxDescriptor = nullptr;

            if (m_gfxUniforms) m_gfxUniforms->free();
            m_gfxUniforms = nullptr;

            if (m_pipeline) delete m_pipeline;
            m_pipeline = nullptr;

            shutdownRendering();

            if (m_window) delete m_window;
            m_window = nullptr;
        }

        virtual bool setupInstance(Instance* instance) {
            //instance->enableValidation();
            instance->subscribeLogger(this);
            return true;
        }

        virtual bool setupDevice(vulkan::LogicalDevice* device) {
            return device->init(true, true, false, getSurface());
        }
        
        const vulkan::PhysicalDevice* choosePhysicalDevice(const ::utils::Array<vulkan::PhysicalDevice>& devices) {
            const vulkan::PhysicalDevice* gpu = nullptr;
            render::vulkan::SwapChainSupport swapChainSupport;

            for (render::u32 i = 0;i < devices.size() && !gpu;i++) {
                if (!devices[i].isDiscrete()) continue;
                if (!devices[i].isExtensionAvailable(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;

                if (!devices[i].getSurfaceSwapChainSupport(getSurface(), &swapChainSupport)) continue;
                if (!swapChainSupport.isValid()) continue;

                if (!swapChainSupport.hasFormat(VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) continue;
                if (!swapChainSupport.hasPresentMode(VK_PRESENT_MODE_FIFO_KHR)) continue;

                auto& capabilities = swapChainSupport.getCapabilities();
                if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < 3) continue;

                gpu = &devices[i];
            }

            return gpu;
        }

        bool setupSwapchain(vulkan::SwapChain* swapChain, const vulkan::SwapChainSupport& support) {
            return swapChain->init(
                getSurface(),
                getLogicalDevice(),
                support,
                VK_FORMAT_A2B10G10R10_UNORM_PACK32,
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
                VK_PRESENT_MODE_FIFO_KHR,
                3
            );
        }

        bool initGraphics() {
            if (!m_window->setOpen(true)) return false;

            ShowCursor(FALSE);
            ShowWindow(GetConsoleWindow(), SW_HIDE);

            if (!initRendering(m_window)) return false;

            constexpr u32 gridLineCount = divisionCount * divisionCount * divisionCount * 12;
            if (!initDebugDrawing(gridLineCount + 4096)) return false;

            m_pipeline = new GraphicsPipeline(
                getShaderCompiler(),
                getLogicalDevice(),
                getSwapChain(),
                getRenderPass()
            );
            m_pipeline->subscribeLogger(this);

            const char* vsh =
                "layout (location = 0) in vec3 v_pos;\n"
                "layout (location = 1) in vec3 v_velocity;\n"
                "layout (location = 2) in vec3 v_acceleration;\n"
                "layout (location = 3) in float v_mass;\n"
                "layout (location = 4) in float v_cellFilledFrac;\n"
                "layout (location = 5) in float v_gridX;\n"
                "layout (location = 6) in float v_gridY;\n"
                "layout (location = 7) in float v_gridZ;\n"
                "\n"
                "layout (binding = 0) uniform _ubo {\n"
                "    mat4 viewProj;\n"
                "    float minSpeed;\n"
                "    float maxSpeed;\n"
                "    float minNormalMass;\n"
                "    float maxNormalMass;\n"
                "    float minMassiveMass;\n"
                "    float maxMassiveMass;\n"
                "} ubo;\n"
                "\n"
                "layout (location = 1) out vec4 a_color;\n"
                "layout (location = 2) out float a_depth;\n"
                "\n"
                "vec4 hsv2rgb(vec3 c) {\n"
                "    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);\n"
                "    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);\n"
                "    return vec4(c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y), 1.0);\n"
                "}\n"
                "\n"
                "void main() {\n"
                "    vec4 pos = ubo.viewProj * vec4(v_pos, 1.0);\n"
                "    float accelFac = length(v_acceleration) * 0.1;\n"
                "    gl_PointSize = min(max(accelFac, 2.5), 8.0);\n"
                "    gl_Position = pos;\n"
                "    float speedFac = smoothstep(length(v_velocity), 0.0, 6.0);\n"
                "    a_color = hsv2rgb(vec3(speedFac + 0.6, smoothstep(accelFac, 3.0, 8.0), 1.0));\n"
                "    // a_color = hsv2rgb(vec3(v_cellFilledFrac * 360.0, 1.0, 1.0));\n"
                "    // a_color = vec4(v_gridX, v_gridY, v_gridZ, 1.0);\n"
                "    a_depth = (pos.z + 1.0) * 0.5;\n"
                "}\n"
            ;
            const char* fsh =
                "layout (location = 1) in vec4 a_color;\n"
                "layout (location = 2) in float a_depth;\n"
                "\n"
                "layout (location = 0) out vec4 o_color;\n"
                "\n"
                "void main() {\n"
                "    float f = length(gl_PointCoord - vec2(0.5));\n"
                "    if (f > 0.5) discard;\n"
                "\n"
                "    vec4 color = a_color * vec4(1.0, 1.0, 1.0, 1.0 - (f + 0.5));\n"
                "    color.w *= 0.85;\n"
                "    o_color = color;\n"
                "}\n"
            ;
            
            m_vfmt.addAttr(&particle::pos);
            m_vfmt.addAttr(&particle::velocity);
            m_vfmt.addAttr(&particle::acceleration); 
            m_vfmt.addAttr(&particle::mass);
            m_vfmt.addAttr(&particle::cellFilledFrac);
            m_vfmt.addAttr(&particle::gridX);
            m_vfmt.addAttr(&particle::gridY);
            m_vfmt.addAttr(&particle::gridZ);
            m_vfmt.setSize(sizeof(particle));
            m_pipeline->setVertexFormat(&m_vfmt);

            m_pipeline->addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
            m_pipeline->addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
            m_pipeline->setPrimitiveType(PT_POINTS);
            // m_pipeline->setDepthTestEnabled(true);
            // m_pipeline->setDepthCompareOp(COMPARE_OP::CO_LESS_OR_EQUAL);
            // m_pipeline->setDepthWriteEnabled(true);
            m_pipeline->setColorBlendEnabled(true);
            m_pipeline->setColorBlendOp(BLEND_OP::BO_ADD);
            m_pipeline->setAlphaBlendOp(BLEND_OP::BO_ADD);
            m_pipeline->setSrcColorBlendFactor(BLEND_FACTOR::BF_SRC_ALPHA);
            m_pipeline->setDstColorBlendFactor(BLEND_FACTOR::BF_ONE_MINUS_SRC_ALPHA);
            m_pipeline->setSrcAlphaBlendFactor(BLEND_FACTOR::BF_ONE);
            m_pipeline->setDstAlphaBlendFactor(BLEND_FACTOR::BF_ONE_MINUS_SRC_ALPHA);

            m_gufmt.addAttr(&render_uniforms::viewProj);
            m_gufmt.addAttr(&render_uniforms::minSpeed);
            m_gufmt.addAttr(&render_uniforms::maxSpeed);
            m_gufmt.addAttr(&render_uniforms::minNormalMass);
            m_gufmt.addAttr(&render_uniforms::maxNormalMass);
            m_gufmt.addAttr(&render_uniforms::minMassiveMass);
            m_gufmt.addAttr(&render_uniforms::maxMassiveMass);
            m_pipeline->addUniformBlock(0, &m_gufmt, VK_SHADER_STAGE_VERTEX_BIT);

            if (!m_pipeline->setVertexShader(vsh)) return false;
            if (!m_pipeline->setFragmentShader(fsh)) return false;
            if (!m_pipeline->init()) return false;

            m_gfxUniforms = allocateUniformObject(&m_gufmt);
            if (!m_gfxUniforms) return false;

            m_gfxDescriptor = allocateDescriptor(m_pipeline);
            if (!m_gfxDescriptor) return false;
            m_gfxDescriptor->add(m_gfxUniforms, 0);
            m_gfxDescriptor->update();

            return true;
        }

        bool initCompute() {
            m_simStep.m_device = getLogicalDevice();
            m_simStep.m_ctx = &m_simCtx;
            m_optStep.m_device = getLogicalDevice();
            m_optStep.m_ctx = &m_simCtx;

            bool r;

            m_simCtx.particlesIn = new Buffer(getLogicalDevice());
            r = m_simCtx.particlesIn->init(
                particleCount * sizeof(particle),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_SHARING_MODE_EXCLUSIVE,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            if (!r) return false;

            m_simCtx.particlesOut = new Buffer(getLogicalDevice());
            r = m_simCtx.particlesOut->init(
                particleCount * sizeof(particle),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_SHARING_MODE_EXCLUSIVE,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            m_simCtx.particleGrid = new Buffer(getLogicalDevice());
            r = m_simCtx.particleGrid->init(
                gridSizeInBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_SHARING_MODE_EXCLUSIVE,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );

            m_simCtx.particleGridOut = new Buffer(getLogicalDevice());
            r = m_simCtx.particleGridOut->init(
                readGridSizeInBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_SHARING_MODE_EXCLUSIVE,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            if (!r || !m_simCtx.particleGridOut->map()) return false;

            m_simCtx.commandPool = new CommandPool(
                getLogicalDevice(),
                &getLogicalDevice()->getComputeQueue()->getFamily()
            );

            if (!m_simCtx.commandPool->init(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)) return false;

            m_simCtx.commandBuf = m_simCtx.commandPool->createBuffer(true);
            if (!m_simCtx.commandBuf) return false;
            
            if (!m_simStep.init(this)) return false;
            if (!m_optStep.init(this)) return false;

            return true;
        }

        void updateRenderUniforms(CommandBuffer* cb) {
            m_gfxUniforms->set<render_uniforms>({
                m_view * m_projection,
                m_minObservedSpeed,
                m_maxObservedSpeed,
                minNormalMass,
                maxNormalMass,
                minMassiveMass,
                maxMassiveMass
            });
            m_gfxUniforms->getBuffer()->submitUpdates(cb);
        }

        bool initialize() {
            if (!initGraphics()) return false;
            if (!initCompute()) return false;
            
            return true;
        }

        bool service() {
            if (!m_window->isOpen()) return false;

            m_window->pollEvents();

            f32 dt = m_frameTimer;
            m_frameTimer.reset();
            m_frameTimer.start();

            if (dt == 0.0f) dt = 1.0f / 60.0f;
            f32 msf = 1000.0f * dt;
            f32 fps = 1.0f / dt;
            m_window->setTitle(String::Format("n-body | %0.2f f/s | %0.4f ms/f", fps, msf));

            auto e = getSwapChain()->getExtent();
            m_projection = mat4f::Perspective(radians(70.0f), f32(e.width) / f32(e.height), 0.1f, 10000.0f);

            f32 camVerticalPos = sinf(radians(m_runTime.elapsed() * cameraVerticalOscillateSpeed)) * universeSize * cameraVerticalOscillateRangeFactor;
            f32 camDistance = cameraBaseDistance + (cosf(radians(m_runTime.elapsed() * cameraInwardOscillateSpeed)) * universeSize * cameraInwardOscillateRangeFactor);

            vec3f eye = mat3f::Rotation(vec3f(0, 1, 0), radians(m_runTime.elapsed() * cameraRotationSpeed)) * vec3f(camDistance, camVerticalPos, camDistance);
            m_view = mat4f::LookAt(eye, vec3f(0.0f, 0.0f, 0.0f), vec3f(0.0f, 1.0f, 0.0f));

            update(dt * timeMultiplier);
            draw();

            if (m_resetTimer.elapsed() > 8.0f * 60.0f) {
                m_resetTimer.reset();
                m_resetTimer.start();
                m_simStep.initParticles();
            }

            getLogicalDevice()->waitForIdle();
            return true;
        }

        void update(f32 dt) {
            m_optStep.execute();
            m_simStep.execute(dt);
        }

        void draw() {
            auto screenSize = m_pipeline->getSwapChain()->getExtent();

            auto frame = getFrame();
            frame->begin();
            auto cb = frame->getCommandBuffer();

            updateRenderUniforms(cb);

            frame->setClearColor(0, vec4f(0.0f, 0.0f, 0.0f, 1.0f));
            frame->setClearDepthStencil(1, 1.0f, 0);

            auto draw = getDebugDraw();
            draw->setProjection(m_projection);
            draw->setView(m_view);
            draw->begin(frame->getSwapChainImageIndex());
            draw->box(
                vec3f(-universeSize, -universeSize, -universeSize),
                vec3f(universeSize, universeSize, universeSize),
                vec4f(1.0f, 1.0f, 1.0f, 0.06f)
            );

            if constexpr (renderGrid) {
                m_simCtx.particleGridOut->fetch(0, readGridSizeInBytes);
                cell_read_data* cells = (cell_read_data*)m_simCtx.particleGridOut->getPointer();

                vec3f offset = vec3f(-universeSize, -universeSize, -universeSize);
                for (u32 x = 0;x < divisionCount;x++) {
                    for (u32 y = 0;y < divisionCount;y++) {
                        for (u32 z = 0;z < divisionCount;z++) {
                            cell_read_data& cell = cells[(z * divisionCount * divisionCount) + (y * divisionCount) + x];
                            vec3f minCorner = vec3f(x, y, z) * cellSize;
                            vec3f maxCorner = minCorner + cellSize;

                            f32 cellFillFac = f32(cell.particleCount) / f32(maxParticlesPerCell);
                            vec4f color;
                            if (cell.particleCount == maxParticlesPerCell) color = vec4f(1.0f, 0.0f, 1.0f, gridAlphaFactor);
                            else if ((cellFillFac * gridAlphaFactor) < 0.002f) continue;
                            else color = vec4f::HSV((1.0f - cellFillFac) * 100.0f, 1.0f, 1.0f, cellFillFac * gridAlphaFactor);

                            draw->box(minCorner + offset, maxCorner + offset, color);
                        }
                    }
                }
            }
            draw->end(cb);

            cb->beginRenderPass(m_pipeline, frame->getFramebuffer());
            draw->draw(cb);
            
            cb->bindPipeline(m_pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);

            cb->setViewport(0, screenSize.height, screenSize.width, -f32(screenSize.height), 0, 1);
            cb->setScissor(0, 0, screenSize.width, screenSize.height);

            cb->bindVertexBuffer(m_simCtx.particlesOut);
            cb->bindDescriptorSet(m_gfxDescriptor, VK_PIPELINE_BIND_POINT_GRAPHICS);
            cb->draw(particleCount);

            cb->endRenderPass();
            frame->end();

            releaseFrame(frame);
        }

        void maybeClose() {
            if (m_runTime < 2.5f) return;
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
        Timer m_runTime;
        Timer m_frameTimer;
        Timer m_resetTimer;
        f32 m_minObservedSpeed;
        f32 m_maxObservedSpeed;
        mat4f m_projection;
        mat4f m_view;

        // Compute
        sim_context m_simCtx;
        OptimizeStep m_optStep;
        SimulateStep m_simStep;

        // Graphics
        GraphicsPipeline* m_pipeline;
        core::DataFormat m_vfmt;
        core::DataFormat m_gufmt;
        DescriptorSet* m_gfxDescriptor;
        UniformObject* m_gfxUniforms;
};

void doTheThing() {
    auto monitors = Window::GetMonitors();
    if (monitors.size() > 0) {
        ::utils::Array<Screensaver*> screens;

        // for (u32 i = 0;i < monitors.size();i++) {
        //     screens.push(new Screensaver(&monitors[i]));
        // }

        screens.push(new Screensaver(monitors.find([](const MonitorInfo& m) {
            return m.isPrimary;
        })));

        for (u32 i = 0;i < screens.size();i++) {
            if (!screens[i]->initialize()) {
                for (u32 s = 0;s < screens.size();s++) delete screens[s];
                return;
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

int main(int argc, char** argv) {
    srand(time(nullptr));

    Mem::Create();
    // ThreadPool::Create();
    doTheThing();
    // ThreadPool::Destroy();
    Mem::Destroy();

    return 0;
}