#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <stdlib.h>
#include <math.h>

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

#ifndef _MSC_VER
#   define SYS_NO_RETURN   __attribute__ ((__noreturn__))
#else
#   define SYS_NO_RETURN    
#endif

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

static inline double double_min( double x, double y )
{
	return x < y ? x : y;
}

static inline double double_max( double x, double y )
{
	return x > y ? x : y;
}

static inline double double_lerp( double a, double b, double x )
{
	return a + ( b - a ) * x;
}

static inline double double_clamp( double x, double min, double max )
{
	return double_min( double_max( x, min ), max );
}

static inline float float_saturate( float x )
{
    return float_clamp( x, 0.0f, 1.0f );
}

static inline float float_lerp( float a, float b, float x )
{
    return a + ( b - a ) * x;
}

static inline float float_rand()
{
    return (float)rand()/(float)RAND_MAX;
}

static inline float float_rand_range( float min, float max )
{
    return float_lerp( min, max, float_rand() );
}

static inline float float_rand_normal( float mean, float sd )
{
    const float x0 = float_rand();
    const float x1 = float_rand();
    
    const float y = sqrtf( -2.0f * logf( x0 ) ) * sinf( 2.0f * (float)PI * x1 );
    return mean + y * sd;
}

static inline float2* float2_rand_normal( float2* pValue, float mean, float sd )
{
    const float x = float_rand_normal( mean, sd );
    const float y = float_rand_normal( mean, sd );

    pValue->x = x;
    pValue->y = y;
    return pValue;
}

static inline float2* float2_scale( float2* pValue, float factor )
{
    const float x = pValue->x;
    const float y = pValue->y;

    const float rx = x * factor;
    const float ry = y * factor;

    pValue->x = rx;
    pValue->y = ry;
    return pValue;
}

static inline float2* float2_add( float2* pResult, const float2* pA, const float2* pB )
{
    const float ax = pA->x;
    const float ay = pA->y;
    const float bx = pB->x;
    const float by = pB->y;

    const float rx = ax + bx;
    const float ry = ay + by;
    
    pResult->x = rx;
    pResult->y = ry;

    return pResult;
}

static inline float2* float2_sub( float2* pResult, const float2* pA, const float2* pB )
{
    const float ax = pA->x;
    const float ay = pA->y;
    const float bx = pB->x;
    const float by = pB->y;

    const float rx = ax - bx;
    const float ry = ay - by;
    
    pResult->x = rx;
    pResult->y = ry;

    return pResult;
}

static inline float2* float2_set( float2* pValue, float x, float y )
{
    pValue->x = x;
    pValue->y = y;
    return pValue;
}

static inline float float2_squareLength( const float2* pX )
{
    const float x = pX->x;
    const float y = pX->y;

    return x*x+y*y;
}

static inline float float2_length( const float2* pX )
{
    return sqrtf(float2_squareLength(pX));
}

static inline float float2_squareDistance( const float2* pA, const float2* pB )
{
    const float ax = pA->x;
    const float ay = pA->y;
    const float bx = pB->x;
    const float by = pB->y;

    const float dx = ax-bx;
    const float dy = ay-by;

    return dx*dx+dy*dy;
}

static inline float float2_distance( const float2* pA, const float2* pB )
{
    return sqrtf(float2_squareDistance(pA,pB));
}

static inline float2* float2_normalize( float2* pValue )
{
    const float invLength = 1.0f / float2_length(pValue);
    float2_scale( pValue, invLength );
    return pValue;
}

static inline float2* float2_perpendicular( float2* pResult, const float2* pA )
{
    const float x = pA->x;
    const float y = pA->y;

    const float rx = y;
    const float ry = -x;

    pResult->x = rx;
    pResult->y = ry;
    return pResult;
}

// r = a + b * c
static inline float2* float2_addScaled2f( float2* pResult, const float2* pA, const float2* pB, const float2* pC )
{
    const float ax = pA->x;
    const float ay = pA->y;
    const float bx = pB->x;
    const float by = pB->y;
    const float cx = pC->x;
    const float cy = pC->y;

    const float rx = ax + bx * cx;
    const float ry = ay + by * cy;

    pResult->x = rx;
    pResult->y = ry;
    return pResult;
}

static inline float2* float2_addScaled1f( float2* pResult, const float2* pA, const float2* pB, float c )
{
    const float ax = pA->x;
    const float ay = pA->y;
    const float bx = pB->x;
    const float by = pB->y;

    const float rx = ax + bx * c;
    const float ry = ay + by * c;

    pResult->x = rx;
    pResult->y = ry;
    return pResult;
}

static inline float3* float3_set( float3* pValue, float x, float y, float z )
{
    pValue->x = x;
    pValue->y = y;
    pValue->z = z;
    return pValue;
}

static inline float4* float4_set( float4* pValue, float x, float y, float z, float w )
{
    pValue->x = x;
    pValue->y = y;
    pValue->z = z;
    pValue->w = w;
    return pValue;
}

#endif

