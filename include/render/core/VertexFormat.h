#pragma once
#include <render/types.h>

#include <utils/Array.h>

namespace render {
    namespace core {
        class VertexFormat {
            public:
                VertexFormat();
                VertexFormat(const VertexFormat& o);
                ~VertexFormat();

                void addAttr(DATA_TYPE type);

                const utils::Array<DATA_TYPE>& getAttributes() const;
                u32 getSize() const;

                operator bool() const;
                bool operator==(const VertexFormat& rhs) const;
                void operator=(const VertexFormat& rhs);

                static u32 AttributeSize(DATA_TYPE type);
            
            protected:
                utils::Array<DATA_TYPE> m_attrs;
                u32 m_size;
        };
    };
};