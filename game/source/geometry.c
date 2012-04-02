#include "geometry.h"
#include "vector.h"

int isCircleCircleIntersecting( const Circle* pCircleA, const Circle* pCircleB )
{
	return float2_squareDistance( &pCircleA->center, &pCircleB->center ) < float_sqr( pCircleA->radius + pCircleB->radius );
}

static float getLinePointDistance( const Line* pLine, const float2* pPoint )
{
	float2 line;
	float2_sub( &line, &pLine->b, &pLine->a );

	float2 point;
	float2_sub( &point, pPoint, &pLine->a );

	const float lineLength = float2_length( &line );
	if( lineLength < 0.0001f )
	{
		return float2_length( &point );
	}
	else
	{
		return float_abs( float2_dot( &point, &line ) ) / lineLength;
	}
}

int isCircleLineIntersecting( const Circle* pCircle, const Line* pLine )
{
	return getLinePointDistance( pLine, &pCircle->center ) <= pCircle->radius;
}

int isCircleCapsuleIntersecting( const Circle* pCircle, const Capsule* pCapsule )
{
	return getLinePointDistance( &pCapsule->line, &pCircle->center ) <= pCircle->radius + pCapsule->radius;
}