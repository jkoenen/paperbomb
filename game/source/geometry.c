#include "geometry.h"
#include "vector.h"

int isCircleCircleIntersecting( const Circle* pCircleA, const Circle* pCircleB )
{
	return float2_squareDistance( &pCircleA->center, &pCircleB->center ) < float_sqr( pCircleA->radius + pCircleB->radius );
}

int isCircleLineIntersecting( const Circle* pCircle, const Line* pLine )
{
	float2 line;
	float2_sub( &line, &pLine->b, &pLine->a );

	float2 circle;
	float2_sub( &circle, &pCircle->center, &pLine->a );

	float distance;

	const float lineLength = float2_length( &line );
	if( lineLength < 0.00001f )
	{
		distance = float2_length( &circle );
	}
	else
	{

		distance = float_abs( circle.x * line.y - circle.y * line.x ) / sqrtf( float_sqr( line.x ) + float_sqr( line.y ) );
	}

	return distance <= pCircle->radius;
}

int isCircleCapsuleIntersecting( const Circle* pCircle, const Capsule* pCapsule )
{
	float2 line;
	float2_sub( &line, &pCapsule->line.b, &pCapsule->line.a );

	float2 circle;
	float2_sub( &circle, &pCircle->center, &pCapsule->line.a );

	float distance;

	const float lineLength = float2_length( &line );
	if( lineLength < 0.00001f )
	{
		distance = float2_length( &circle );
	}
	else
	{

		distance = float_abs( circle.x * line.y - circle.y * line.x ) / sqrtf( float_sqr( line.x ) + float_sqr( line.y ) );
	}

	return distance <= pCircle->radius + pCapsule->radius;
}

