#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#ifdef _MSC_VER
#if (_MSC_VER < 1300)
   typedef signed char       int8_t;
   typedef signed short      int16_t;
   typedef signed int        int32_t;
   typedef unsigned char     uint8_t;
   typedef unsigned short    uint16_t;
   typedef unsigned int      uint32_t;
#else
   typedef signed __int8     int8_t;
   typedef signed __int16    int16_t;
   typedef signed __int32    int32_t;
   typedef unsigned __int8   uint8_t;
   typedef unsigned __int16  uint16_t;
   typedef unsigned __int32  uint32_t;
#endif
typedef signed __int64       int64_t;
typedef unsigned __int64     uint64_t;

#define inline __inline
#else
#   include <inttypes.h>
#endif

#define SYS_USE_ARGUMENT(x)	(void)x
#define SYS_COUNTOF(x) ((sizeof(x))/sizeof(x[0]))

#define PI 3.14159265

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef unsigned int uint;

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

static inline float float_lerp( float a, float b, float x )
{
    return a + ( b - a ) * x;
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

#endif

