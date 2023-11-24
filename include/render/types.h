#pragma once
#include <stdint.h>

namespace render {
    typedef uint64_t    u64;
    typedef int64_t     i64;
    typedef uint32_t    u32;
    typedef int32_t     i32;
    typedef uint16_t    u16;
    typedef int16_t     i16;
    typedef uint8_t     u8;
    typedef int8_t      i8;
    typedef float       f32;
    typedef double      f64;

    enum PRIMITIVE_TYPE {
        PT_POINTS,
        PT_LINES,
        PT_LINE_STRIP,
        PT_TRIANGLES,
        PT_TRIANGLE_STRIP,
        PT_TRIANGLE_FAN
    };

    enum POLYGON_MODE {
        PM_FILLED,
        PM_WIREFRAME,
        PM_POINTS
    };

    enum CULL_MODE {
        CM_FRONT_FACE,
        CM_BACK_FACE,
        CM_BOTH_FACES
    };

    enum FRONT_FACE_MODE {
        FFM_CLOCKWISE,
        FFM_COUNTER_CLOCKWISE
    };

    enum COMPARE_OP {
        CO_NEVER,
        CO_ALWAYS,
        CO_LESS,
        CO_LESS_OR_EQUAL,
        CO_GREATER,
        CO_GREATER_OR_EQUAL,
        CO_EQUAL,
        CO_NOT_EQUAL
    };

    enum BLEND_FACTOR {
        BF_ZERO,
        BF_ONE,
        BF_SRC_COLOR,
        BF_ONE_MINUS_SRC_COLOR,
        BF_DST_COLOR,
        BF_ONE_MINUS_DST_COLOR,
        BF_SRC_ALPHA,
        BF_ONE_MINUS_SRC_ALPHA,
        BF_DST_ALPHA,
        BF_ONE_MINUS_DST_ALPHA,
        BF_CONSTANT_COLOR,
        BF_ONE_MINUS_CONSTANT_COLOR,
        BF_CONSTANT_ALPHA,
        BF_ONE_MINUS_CONSTANT_ALPHA,
        BF_SRC_ALPHA_SATURATE,
        BF_SRC1_COLOR,
        BF_ONE_MINUS_SRC1_COLOR,
        BF_SRC1_ALPHA,
        BF_ONE_MINUS_SRC1_ALPHA
    };

    enum BLEND_OP {
        BO_ADD,
        BO_SUBTRACT,
        BO_REVERSE_SUBTRACT,
        BO_MIN,
        BO_MAX
    };
};