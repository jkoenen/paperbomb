#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <inttypes.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef struct
{
    float   x, y;
} float2;

typedef struct
{
    float   x, y, z;
} float3;

typedef struct
{
    float   x, y, z, w;
} float4;


static inline void float2_set( float2* pValue, float x, float y )
{
    pValue->x = x;
    pValue->y = y;
}

static inline void float3_set( float3* pValue, float x, float y, float z )
{
    pValue->x = x;
    pValue->y = y;
    pValue->z = z;
}

static inline void float4_set( float4* pValue, float x, float y, float z, float w )
{
    pValue->x = x;
    pValue->y = y;
    pValue->z = z;
    pValue->w = w;
}

static inline float float_min( float x, float y )
{
    return x < y ? x : y;
}

static inline float float_max( float x, float y )
{
    return x > y ? x : y;
}

static inline float float_clamp( float x, float min, float max )
{
    return float_min( float_max( x, min ), max );
}

static inline float float_saturate( float x )
{
    return float_clamp( x, 0.0f, 1.0f );
}

static inline void float2_scale( float2* pValue, float factor )
{
    pValue->x *= factor;
    pValue->y *= factor;
}

static inline void float2_add( float2* pResult, const float2* pA, const float2* pB )
{
    pResult->x = pA->x + pB->x;
    pResult->y = pA->y + pB->y;
}

#endif

