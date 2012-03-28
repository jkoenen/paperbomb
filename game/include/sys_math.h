#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

typedef struct float4
{
    float   x, y, z, w;
} float4_t;

static inline float4_t float4_create( float x, float y, float z, float w )
{
    float4_t result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

#endif

