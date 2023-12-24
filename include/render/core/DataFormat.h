#pragma once
#include <render/types.h>

#include <utils/Math.hpp>
#include <utils/Array.h>

namespace render {
    namespace core {
        class DataFormat {
            public:
                struct Attribute {
                    DATA_TYPE type;
                    const DataFormat* formatRef;
                    u32 elementCount;
                    u32 offset;
                    u32 size;
                    u32 uniformAlignedSize;
                };

                DataFormat();
                DataFormat(const DataFormat& o);
                ~DataFormat();

                void addAttr(DATA_TYPE type, u32 offset, u32 elementCount = 1);
                void addAttr(const DataFormat* type, u32 offset, u32 elementCount = 1);

                #define addAttribute(type) addAttr(type, (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr, 1)
                template <typename Cls> void addAttr(i32 Cls::* member) { addAttribute(dt_int); }
                template <typename Cls> void addAttr(f32 Cls::* member) { addAttribute(dt_float); }
                template <typename Cls> void addAttr(u32 Cls::* member) { addAttribute(dt_uint); }
                template <typename Cls> void addAttr(vec2i Cls::* member) { addAttribute(dt_vec2i); }
                template <typename Cls> void addAttr(vec2f Cls::* member) { addAttribute(dt_vec2f); }
                template <typename Cls> void addAttr(vec2ui Cls::* member) { addAttribute(dt_vec2ui); }
                template <typename Cls> void addAttr(vec3i Cls::* member) { addAttribute(dt_vec3i); }
                template <typename Cls> void addAttr(vec3f Cls::* member) { addAttribute(dt_vec3f); }
                template <typename Cls> void addAttr(vec3ui Cls::* member) { addAttribute(dt_vec3ui); }
                template <typename Cls> void addAttr(vec4i Cls::* member) { addAttribute(dt_vec4i); }
                template <typename Cls> void addAttr(vec4f Cls::* member) { addAttribute(dt_vec4f); }
                template <typename Cls> void addAttr(vec4ui Cls::* member) { addAttribute(dt_vec4ui); }
                template <typename Cls> void addAttr(mat2f Cls::* member) { addAttribute(dt_mat2f); }
                template <typename Cls> void addAttr(mat3f Cls::* member) { addAttribute(dt_mat3f); }
                template <typename Cls> void addAttr(mat4f Cls::* member) { addAttribute(dt_mat4f); }
                #undef addAttribute

                #define addAttribute(type) addAttr(type, (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr, N)
                template <typename Cls, int N> void addAttr(i32 (Cls::* member)[N]) { addAttribute(dt_int); }
                template <typename Cls, int N> void addAttr(f32 (Cls::* member)[N]) { addAttribute(dt_float); }
                template <typename Cls, int N> void addAttr(u32 (Cls::* member)[N]) { addAttribute(dt_uint); }
                template <typename Cls, int N> void addAttr(vec2i (Cls::* member)[N]) { addAttribute(dt_vec2i); }
                template <typename Cls, int N> void addAttr(vec2f (Cls::* member)[N]) { addAttribute(dt_vec2f); }
                template <typename Cls, int N> void addAttr(vec2ui (Cls::* member)[N]) { addAttribute(dt_vec2ui); }
                template <typename Cls, int N> void addAttr(vec3i (Cls::* member)[N]) { addAttribute(dt_vec3i); }
                template <typename Cls, int N> void addAttr(vec3f (Cls::* member)[N]) { addAttribute(dt_vec3f); }
                template <typename Cls, int N> void addAttr(vec3ui (Cls::* member)[N]) { addAttribute(dt_vec3ui); }
                template <typename Cls, int N> void addAttr(vec4i (Cls::* member)[N]) { addAttribute(dt_vec4i); }
                template <typename Cls, int N> void addAttr(vec4f (Cls::* member)[N]) { addAttribute(dt_vec4f); }
                template <typename Cls, int N> void addAttr(vec4ui (Cls::* member)[N]) { addAttribute(dt_vec4ui); }
                template <typename Cls, int N> void addAttr(mat2f (Cls::* member)[N]) { addAttribute(dt_mat2f); }
                template <typename Cls, int N> void addAttr(mat3f (Cls::* member)[N]) { addAttribute(dt_mat3f); }
                template <typename Cls, int N> void addAttr(mat4f (Cls::* member)[N]) { addAttribute(dt_mat4f); }
                #undef addAttribute
                
                template <typename Cls, typename T>
                std::enable_if_t<std::is_class_v<T>, void>
                addAttr(T Cls::* member, const DataFormat* type) {
                    addAttr(type, (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr, 1);
                }
                
                template <typename Cls, typename T, int N>
                std::enable_if_t<std::is_class_v<T>, void>
                addAttr(T (Cls::* member)[N], const DataFormat* type) {
                    addAttr(type, (u8*)&((Cls*)nullptr->*member) - (u8*)nullptr, N);
                }

                const Array<Attribute>& getAttributes() const;

                void setSize(u32 size);
                u32 getSize() const;
                u32 getUniformBlockSize() const;
                bool isValid() const;
                bool isEqualTo(const DataFormat* format) const;

                static u32 AttributeSize(DATA_TYPE type, bool uniformAligned = false);
            
            protected:
                Array<Attribute> m_attrs;
                u32 m_size;
                u32 m_uniformBlockSize;
        };
    };
};