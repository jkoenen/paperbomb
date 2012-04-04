#ifndef VECTOR_H_INCLUDE
#define VECTOR_H_INCLUDE

#include "types.h"

static inline float2* float2_set( float2* pValue, float x, float y )
{
	pValue->x = x;
	pValue->y = y;
	return pValue;
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

static inline float2* float2_rand_normal( float2* pValue, float mean, float sd )
{
	const float x = float_rand_normal( mean, sd );
	const float y = float_rand_normal( mean, sd );

	pValue->x = x;
	pValue->y = y;
	return pValue;
}

static inline float2* float2_scale1f( float2* pValue, float factor )
{
	const float x = pValue->x;
	const float y = pValue->y;

	const float rx = x * factor;
	const float ry = y * factor;

	pValue->x = rx;
	pValue->y = ry;
	return pValue;
}

static inline float2* float2_scale2f( float2* pValue, float2* pFactor )
{
	const float x = pValue->x;
	const float y = pValue->y;

	const float rx = x * pFactor->x;
	const float ry = y * pFactor->y;

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

static inline float2* float2_add2f( float2* pResult, const float2* pA, float bx, float by )
{
	const float ax = pA->x;
	const float ay = pA->y;

	const float rx = ax + bx;
	const float ry = ay + by;

	pResult->x = rx;
	pResult->y = ry;

	return pResult;
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
	float2_scale1f( pValue, invLength );
	return pValue;
}

static inline float2* float2_lerp( float2* pValue, const float2* pA, const float2* pB, float x )
{
	const float ax = pA->x;
	const float ay = pA->y;
	const float bx = pB->x;
	const float by = pB->y;

    const float rx = float_lerp( ax, bx, x );
    const float ry = float_lerp( ay, by, x );
    return float2_set( pValue, rx, ry );
}

static inline float2* float2_normalize0( float2* pValue )
{
	const float length = float2_length( pValue );
	if( length < 0.000001f )
	{
		float2_set( pValue, 0.0f, 0.0f );
	}
	else
	{
		const float invLength = 1.0f / length;
		float2_scale1f( pValue, invLength );
	}
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

static inline void float2_rotate( float2* pValue, float angle )
{
	const float x = pValue->x;
	const float y = pValue->y;

	const float sinAngle = sinf( angle );
	const float cosAngle = cosf( angle );

	const float newX = x * cosAngle - y * sinAngle;
	const float newY = y * cosAngle + x * sinAngle;

	pValue->x = newX;
	pValue->y = newY;
}

static inline float2* float2_from_angle( float2* pTarget, float angle )
{
	pTarget->x = cosf( angle );
	pTarget->y = sinf( angle );
	return pTarget;
}

static inline float float2_dot( const float2* pA, const float2* pB )
{
	const float ax = pA->x;
	const float ay = pA->y;
	const float bx = pB->x;
	const float by = pB->y;
	return ax*bx+ay*by;
}

static inline int float2_isEqual( const float2* pA, const float2* pB )
{
	const float ax = pA->x;
	const float ay = pA->y;
	const float bx = pB->x;
	const float by = pB->y;
    return float_isEqual( ax, bx ) && float_isEqual( ay, by );
}

#endif

