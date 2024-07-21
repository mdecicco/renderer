#include <render/vulkan/DescriptorSet.h>
#include <render/vulkan/LogicalDevice.h>
#include <render/vulkan/Instance.h>
#include <render/vulkan/Pipeline.h>
#include <render/vulkan/Texture.h>
#include <render/vulkan/Buffer.h>
#include <render/vulkan/UniformBuffer.h>

#include <utils/Array.hpp>

namespace render {
    namespace vulkan {
        //
        // DescriptorPool
        //

        DescriptorPool::DescriptorPool(LogicalDevice* device, u32 maxSets) {
            m_device = device;
            m_maxSets = maxSets;
            m_usedSets = 0;
            m_pool = VK_NULL_HANDLE;
            m_sets = new DescriptorSet[maxSets];

            resetNodes();
        }

        DescriptorPool::~DescriptorPool() {
            shutdown();

            delete [] m_sets;
            m_sets = nullptr;
        }

        LogicalDevice* DescriptorPool::getDevice() const {
            return m_device;
        }

        u32 DescriptorPool::getCapacity() const {
            return m_maxSets;
        }

        u32 DescriptorPool::getRemaining() const {
            return m_maxSets - m_usedSets;
        }

        bool DescriptorPool::init() {
            if (m_pool) return false;

            VkDescriptorPoolSize ps = {};
            ps.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            ps.descriptorCount = m_maxSets;

            VkDescriptorPoolCreateInfo pi = {};
            pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pi.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pi.poolSizeCount = 1;
            pi.pPoolSizes = &ps;
            pi.maxSets = m_maxSets;

            if (vkCreateDescriptorPool(m_device->get(), &pi, m_device->getInstance()->getAllocator(), &m_pool) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to create descriptor pool for samplers");
                shutdown();
                return false;
            }

            return true;
        }

        void DescriptorPool::shutdown() {
            if (m_pool) {
                vkDestroyDescriptorPool(m_device->get(), m_pool, m_device->getInstance()->getAllocator());
                m_pool = VK_NULL_HANDLE;
            }

            resetNodes();
        }

        DescriptorSet* DescriptorPool::allocate(Pipeline* pipeline) {
            if (!m_freeList) return nullptr;

            VkDescriptorSet set = VK_NULL_HANDLE;
            VkDescriptorSetLayout layout = pipeline->getDescriptorSetLayout();

            VkDescriptorSetAllocateInfo ai = {};
            ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            ai.descriptorPool = m_pool;
            ai.descriptorSetCount = 1;
            ai.pSetLayouts = &layout;

            if (vkAllocateDescriptorSets(m_device->get(), &ai, &set) != VK_SUCCESS) {
                m_device->getInstance()->error("Failed to allocate descriptor set for sampler");
                return nullptr;
            }

            DescriptorSet* s = getNode();
            if (!s) return nullptr;

            s->m_set = set;

            return s;
        }

        void DescriptorPool::free(DescriptorSet* s) {
            if (!s || s->m_pool != this) return;

            vkFreeDescriptorSets(m_device->get(), m_pool, 1, &s->m_set);

            freeNode(s);
        }

        void DescriptorPool::resetNodes() {
            for (u32 i = 0;i < m_maxSets;i++) {
                m_sets[i].m_pool = this;
                m_sets[i].m_set = VK_NULL_HANDLE;
                m_sets[i].m_descriptors.clear();
                
                if (i == 0) m_sets[i].m_last = nullptr;
                else m_sets[i].m_last = &m_sets[i - 1];

                if (i == m_maxSets - 1) m_sets[i].m_next = nullptr;
                else m_sets[i].m_next = &m_sets[i + 1];
            }

            m_freeList = m_sets;
            m_usedList = nullptr;
        }

        DescriptorSet* DescriptorPool::getNode() {
            if (!m_freeList) return nullptr;

            DescriptorSet* n = m_freeList;

            m_freeList = n->m_next;
            if (m_freeList) m_freeList->m_last = nullptr;
            
            n->m_next = m_usedList;
            if (m_usedList) m_usedList->m_last = n;
            m_usedList = n;
            
            n->m_set = VK_NULL_HANDLE;
            n->m_descriptors.clear();

            m_usedSets++;
            return n;
        }

        void DescriptorPool::freeNode(DescriptorSet* n) {
            if (!m_usedList) return;

            if (n->m_last) n->m_last->m_next = n->m_next;
            if (n->m_next) n->m_next->m_last = n->m_last;

            if (m_usedList == n) m_usedList = n->m_next;
            
            if (m_freeList) m_freeList->m_last = n;
            n->m_next = m_freeList;
            n->m_last = nullptr;
            m_freeList = n;

            m_usedSets--;
        }



        //
        // DescriptorSet
        //

        DescriptorSet::DescriptorSet() {
            m_pool = nullptr;
            m_next = nullptr;
            m_last = nullptr;
            m_set = VK_NULL_HANDLE;
        }

        DescriptorPool* DescriptorSet::getPool() const {
            return m_pool;
        }

        VkDescriptorSet DescriptorSet::get() const {
            return m_set;
        }

        void DescriptorSet::add(Texture* tex, u32 bindingIndex) {
            m_descriptors.push({
                nullptr,
                tex,
                nullptr,
                bindingIndex
            });
        }

        void DescriptorSet::add(UniformObject* uo, u32 bindingIndex) {
            m_descriptors.push({
                uo,
                nullptr,
                nullptr,
                bindingIndex
            });
        }

        void DescriptorSet::add(Buffer* storageBuffer, u32 bindingIndex) {
            m_descriptors.push({
                nullptr,
                nullptr,
                storageBuffer,
                bindingIndex
            });
        }

        void DescriptorSet::set(Texture* tex, u32 bindingIndex) {
            for (u32 i = 0;i < m_descriptors.size();i++) {
                if (m_descriptors[i].bindingIdx == bindingIndex) {
                    m_descriptors[i] = {
                        nullptr,
                        tex,
                        nullptr,
                        bindingIndex
                    };
                    break;
                }
            }
        }

        void DescriptorSet::set(UniformObject* uo, u32 bindingIndex) {
            for (u32 i = 0;i < m_descriptors.size();i++) {
                if (m_descriptors[i].bindingIdx == bindingIndex) {
                    m_descriptors[i] = {
                        uo,
                        nullptr,
                        nullptr,
                        bindingIndex
                    };
                    break;
                }
            }
        }

        void DescriptorSet::set(Buffer* storageBuffer, u32 bindingIndex) {
            for (u32 i = 0;i < m_descriptors.size();i++) {
                if (m_descriptors[i].bindingIdx == bindingIndex) {
                    m_descriptors[i] = {
                        nullptr,
                        nullptr,
                        storageBuffer,
                        bindingIndex
                    };
                    break;
                }
            }
        }

        void DescriptorSet::update() {
            u32 uboCount = 0;
            u32 texCount = 0;
            u32 sboCount = 0;

            for (u32 i = 0;i < m_descriptors.size();i++) {
                if (m_descriptors[i].uniform) {
                    uboCount++;
                    continue;
                }

                if (m_descriptors[i].tex) {
                    texCount++;
                    continue;
                }

                if (m_descriptors[i].storageBuffer) {
                    sboCount++;
                    continue;
                }
            }

            Array<VkDescriptorBufferInfo> bi(uboCount + sboCount);
            Array<VkDescriptorImageInfo> ii(texCount);
            Array<VkWriteDescriptorSet> writes(m_descriptors.size());

            for (u32 i = 0;i < m_descriptors.size();i++) {
                if (m_descriptors[i].uniform) {
                    auto u = m_descriptors[i].uniform;

                    bi.push({});
                    auto& di = bi.last();
                    di.buffer = u->getBuffer()->getBuffer();

                    u32 offset, size;
                    u->getRange(&offset, &size);
                    di.offset = offset;
                    di.range = size;

                    writes.push({});
                    auto& wd = writes.last();
                    wd.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    wd.dstSet = m_set;
                    wd.dstBinding = m_descriptors[i].bindingIdx;
                    wd.dstArrayElement = 0;
                    wd.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    wd.descriptorCount = 1;
                    wd.pBufferInfo = &di;

                    continue;
                }

                if (m_descriptors[i].tex) {
                    auto t = m_descriptors[i].tex;

                    ii.push({});
                    auto& di = ii.last();
                    di.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    di.imageView = t->getView();
                    di.sampler = t->getSampler();

                    writes.push({});
                    auto& wd = writes.last();
                    wd.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    wd.dstSet = m_set;
                    wd.dstBinding = m_descriptors[i].bindingIdx;
                    wd.dstArrayElement = 0;
                    wd.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    wd.descriptorCount = 1;
                    wd.pImageInfo = &di;

                    continue;
                }
            
                if (m_descriptors[i].storageBuffer) {
                    auto b = m_descriptors[i].storageBuffer;

                    bi.push({});
                    auto& di = bi.last();
                    di.buffer = b->get();
                    di.offset = 0;
                    di.range = VK_WHOLE_SIZE;

                    writes.push({});
                    auto& wd = writes.last();
                    wd.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    wd.dstSet = m_set;
                    wd.dstBinding = m_descriptors[i].bindingIdx;
                    wd.dstArrayElement = 0;
                    wd.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    wd.descriptorCount = 1;
                    wd.pBufferInfo = &di;

                    continue;
                }
            }

            vkUpdateDescriptorSets(m_pool->getDevice()->get(), writes.size(), writes.data(), 0, nullptr);
        }

        void DescriptorSet::free() {
            m_pool->free(this);
        }
    


        //
        // DescriptorFactory
        //

        DescriptorFactory::DescriptorFactory(LogicalDevice* device, u32 maxSetsPerPool) {
            m_device = device;
            m_maxSetsPerPool = maxSetsPerPool;
        }

        DescriptorFactory::~DescriptorFactory() {
            for (u32 i = 0;i < m_pools.size();i++) delete m_pools[i];
        }

        DescriptorSet* DescriptorFactory::allocate(Pipeline* pipeline) {
            DescriptorPool* found = m_pools.find([](DescriptorPool* p) {
                return p->getRemaining() > 0;
            });

            if (found) return found->allocate(pipeline);

            DescriptorPool* p = new DescriptorPool(m_device, m_maxSetsPerPool);
            if (!p->init()) return nullptr;

            m_pools.push(p);
            return p->allocate(pipeline);
        }
    };
};