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
#define SYS_MEMBEROFFSET(s,m)   (size_t)(&(((s*)0)->m))

#define PI 3.14159265
#define GAMETIMESTEP ( 1.0f / 60.0f )

#define DEG2RADF(deg)    ((deg)*(float)PI/180.0f)

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

static inline float float_sqr( float x )
{
	return x * x;
}

static inline float float_abs( float x )
{
	return x < 0.0f ? -x : x;
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
	const float x0 = float_max( 0.0001f, float_rand() );
	const float x1 = float_max( 0.0001f, float_rand() );

	const float y = sqrtf( -2.0f * logf( x0 ) ) * sinf( 2.0f * (float)PI * x1 );
	return mean + y * sd;
}

static inline int float_isEqual( float a, float b )
{
    return a == b;
}

#endif

