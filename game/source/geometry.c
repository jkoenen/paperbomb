#include "geometry.h"
#include "vector.h"

int isCircleCircleIntersecting( const Circle* pCircleA, const Circle* pCircleB )
{
	return float2_squareDistance( &pCircleA->center, &pCircleB->center ) < float_sqr( pCircleA->radius + pCircleB->radius );
}

static float getLinePointDistance( const Line* pLine, const float2* pPoint, float* pLinePos )
{
	float2 ab;
	float2_sub( &ab, &pLine->b, &pLine->a );

	float2 ap;
	float2_sub( &ap, pPoint, &pLine->a );

	const float v = ( ap.x * ab.x + ap.y * ab.y ) / ( ab.x * ab.x + ab.y * ab.y );

	if( pLinePos )
	{
		*pLinePos = v;
	}

	if( v <= 0.0f ) 
	{
		return float2_distance( pPoint, &pLine->a );
	} 
	else if( v >= 1.0f ) 
	{
		return float2_distance( pPoint, &pLine->b );
	} 
	else 
	{
		float2 p;
		float2_addScaled1f( &p, &pLine->a, &ab, v );
		return float2_distance( pPoint, &p );
	}
}

int isCircleLineIntersecting( const Circle* pCircle, const Line* pLine )
{
	return getLinePointDistance( pLine, &pCircle->center, 0 ) <= pCircle->radius;
}

int isCircleCapsuleIntersecting( const Circle* pCircle, const Capsule* pCapsule )
{
	return getLinePointDistance( &pCapsule->line, &pCircle->center, 0 ) <= pCircle->radius + pCapsule->radius;
}

int isCircleCircleIntersectingWithDistance( const Circle* pCircle, const Line* pLine, float* pDistance )
{
	float pos;
	const int result = getLinePointDistance(pLine, &pCircle->center, &pos ) <= pCircle->radius;
	if( result )
	{
		float2 distance;
		float2_sub( &distance, &pLine->b, &pLine->a );
		float2_scale1f( &distance, &distance, pos );
		*pDistance = float2_length( &distance );
	}
	return result;
}

int isLineLineIntersectingWithDistance( const Line* pLineA, const Line* pLineB, float* pDistance )
{
	const float x1 = pLineA->a.x;
	const float x2 = pLineA->b.x;
	const float x3 = pLineB->a.x;
	const float x4 = pLineB->b.x;

	const float y1 = pLineA->a.y;
	const float y2 = pLineA->b.y;
	const float y3 = pLineB->a.y;
	const float y4 = pLineB->b.y;

	const float a = ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 );
	const float b = ( y4 - y3 ) * ( x2 - x1 ) - ( x4 - x3 ) * ( y2 - y1 );

	if( float_abs( b ) < 0.0001f )
	{
		return FALSE;
	}

	const float p = a / b;
	if( ( p < 0.0f ) || ( p > 1.0f ) )
	{
		return FALSE;
	}

	float2 distance;
	float2_sub( &distance, &pLineA->b, &pLineA->a );
	float2_scale1f( &distance, &distance, p );
	*pDistance = float2_length( &distance );

	return TRUE;
}

int circleCircleCollide( const Circle* pFirst, const Circle* pSecond, float massRatio, float2* pFirstPos, float2* pSecondPos )
{
	if( isCircleCircleIntersecting( pFirst, pSecond ) )
	{
		float2 distance;
		float2_sub( &distance, &pSecond->center, &pFirst->center );
		const float distanceLength = float2_length( &distance );
		if( distanceLength > 0.01f )
		{
			const float intersecting = distanceLength - pSecond->radius - pFirst->radius;
			float2_scale1f( &distance, &distance, intersecting / distanceLength );
			
			if( pFirstPos )
			{
				float2_subScaled1f( pFirstPos, pFirstPos, &distance, -( 1.0f - massRatio ) );
			}
			if( pSecondPos )
			{
				float2_subScaled1f( pSecondPos, pSecondPos, &distance, massRatio );
			}
		}

		return TRUE;
	}

	return FALSE;
}

