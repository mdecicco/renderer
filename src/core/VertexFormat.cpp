#include <render/core/VertexFormat.h>

#include <utils/Array.hpp>

namespace render {
    namespace core {
        VertexFormat::VertexFormat() {
            m_size = 0;
        }

        VertexFormat::VertexFormat(const VertexFormat& o) {
            m_attrs = o.m_attrs;
            m_size = o.m_size;
        }

        VertexFormat::~VertexFormat() {
        }

        void VertexFormat::addAttr(DATA_TYPE type) {
            m_attrs.push(type);
            m_size += VertexFormat::AttributeSize(type);
        }

        const utils::Array<DATA_TYPE>& VertexFormat::getAttributes() const {
            return m_attrs;
        }

        u32 VertexFormat::getSize() const {
            return m_size;
        }

        VertexFormat::operator bool() const {
            return m_size > 0;
        }

        bool VertexFormat::operator==(const VertexFormat& rhs) const {
            if (m_size != rhs.m_size) return false;
            if (m_attrs.size() != rhs.m_attrs.size()) return false;

            return !m_attrs.some([this, &rhs](DATA_TYPE tp, u32 idx) {
                return rhs.m_attrs[idx] != tp;
            });
        }

        void VertexFormat::operator=(const VertexFormat& rhs) {
            m_size = rhs.m_size;
            m_attrs = rhs.m_attrs;
        }

        u32 VertexFormat::AttributeSize(DATA_TYPE type) {
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
                sizeof(u32) * 16   // dt_mat4ui
            };

            return dtSizes[type];
        }
    };
};