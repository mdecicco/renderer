#include <render/core/DataFormat.h>

#include <utils/Array.hpp>

namespace render {
    namespace core {
        DataFormat::DataFormat() {
            m_size = 0;
            m_uniformBlockSize = 0;
        }

        DataFormat::DataFormat(const DataFormat& o) {
            m_attrs = o.m_attrs;
            m_size = o.m_size;
            m_uniformBlockSize = o.m_uniformBlockSize;
        }

        DataFormat::~DataFormat() {
        }

        void DataFormat::addAttr(DATA_TYPE type, u32 offset, u32 elementCount) {
            if (type == dt_struct || elementCount == 0) return;

            u32 sz = DataFormat::AttributeSize(type) * elementCount;
            u32 usz = DataFormat::AttributeSize(type, true) * elementCount;
            m_attrs.push({
                type,
                nullptr,
                elementCount,
                offset,
                sz,
                usz
            });

            m_size += sz;
            m_uniformBlockSize += usz;
        }

        void DataFormat::addAttr(const DataFormat* type, u32 offset, u32 elementCount) {
            if (!type || elementCount == 0) return;

            u32 sz = type->m_size * elementCount;
            u32 usz = type->m_uniformBlockSize * elementCount;
            m_attrs.push({
                dt_struct,
                type,
                elementCount,
                offset,
                sz,
                usz
            });

            m_size += sz;
            m_uniformBlockSize += usz;
        }

        const Array<DataFormat::Attribute>& DataFormat::getAttributes() const {
            return m_attrs;
        }

        u32 DataFormat::getSize() const {
            return m_size;
        }
        
        u32 DataFormat::getUniformBlockSize() const {
            return m_uniformBlockSize;
        }

        bool DataFormat::isValid() const {
            return m_size > 0;
        }

        bool DataFormat::isEqualTo(const DataFormat* rhs) const {
            if (m_size != rhs->m_size) return false;
            if (m_attrs.size() != rhs->m_attrs.size()) return false;

            return !m_attrs.some([this, rhs](const DataFormat::Attribute& tp, u32 idx) {
                auto& attr = rhs->m_attrs[idx];
                if (attr.type != tp.type) return true;
                if (attr.elementCount != tp.elementCount) return true;
                if (attr.size != tp.elementCount) return true;
                if (attr.formatRef != tp.formatRef) return true;

                return false;
            });
        }

        u32 DataFormat::AttributeSize(DATA_TYPE type, bool uniformAligned) {
            static u32 dtSizes[dt_enum_count] = {
                sizeof(i32),       // dt_int
                sizeof(f32),       // dt_float
                sizeof(u32),       // dt_uint
                sizeof(i32) * 2,   // dt_vec2i
                sizeof(f32) * 2,   // dt_vec2f
                sizeof(u32) * 2,   // dt_vec2ui
                sizeof(i32) * 3,   // dt_vec3i
                sizeof(f32) * 3,   // dt_vec3f
                sizeof(u32) * 3,   // dt_vec3ui
                sizeof(i32) * 4,   // dt_vec4i
                sizeof(f32) * 4,   // dt_vec4f
                sizeof(u32) * 4,   // dt_vec4ui
                sizeof(i32) * 4,   // dt_mat2i
                sizeof(f32) * 4,   // dt_mat2f
                sizeof(u32) * 4,   // dt_mat2ui
                sizeof(i32) * 9,   // dt_mat3i
                sizeof(f32) * 9,   // dt_mat3f
                sizeof(u32) * 9,   // dt_mat3ui
                sizeof(i32) * 16,  // dt_mat4i
                sizeof(f32) * 16,  // dt_mat4f
                sizeof(u32) * 16,  // dt_mat4ui
                0                  // dt_struct
            };

            static u32 dtUboSizes[dt_enum_count] = {
                sizeof(i32),       // dt_int
                sizeof(f32),       // dt_float
                sizeof(u32),       // dt_uint
                sizeof(i32) * 2,   // dt_vec2i
                sizeof(f32) * 2,   // dt_vec2f
                sizeof(u32) * 2,   // dt_vec2ui
                sizeof(i32) * 4,   // dt_vec3i
                sizeof(f32) * 4,   // dt_vec3f
                sizeof(u32) * 4,   // dt_vec3ui
                sizeof(i32) * 4,   // dt_vec4i
                sizeof(f32) * 4,   // dt_vec4f
                sizeof(u32) * 4,   // dt_vec4ui
                sizeof(i32) * 4,   // dt_mat2i
                sizeof(f32) * 4,   // dt_mat2f
                sizeof(u32) * 4,   // dt_mat2ui
                sizeof(i32) * 16,   // dt_mat3i
                sizeof(f32) * 16,   // dt_mat3f
                sizeof(u32) * 16,   // dt_mat3ui
                sizeof(i32) * 16,  // dt_mat4i
                sizeof(f32) * 16,  // dt_mat4f
                sizeof(u32) * 16,  // dt_mat4ui
                0                  // dt_struct
            };

            return uniformAligned ? dtUboSizes[type] : dtSizes[type];
        }
    };
};