#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <inttypes.h>

typedef struct
{
    float   x, y;
} float2_t;

typedef struct
{
    float   x, y, z;
} float3_t;

typedef struct
{
    float   x, y, z, w;
} float4_t;


static inline void float2_set( float2_t* pValue, float x, float y )
{
    pValue->x = x;
    pValue->y = y;
}

static inline void float3_set( float3_t* pValue, float x, float y, float z )
{
    pValue->x = x;
    pValue->y = y;
    pValue->z = z;
}

static inline void float4_set( float4_t* pValue, float x, float y, float z, float w )
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

static inline void float2_scale( float2_t* pValue, float factor )
{
    pValue->x *= factor;
    pValue->y *= factor;
}

static inline void float2_add( float2_t* pResult, const float2_t* pA, const float2_t* pB )
{
    pResult->x = pA->x + pB->x;
    pResult->y = pA->y + pB->y;
}

#endif

