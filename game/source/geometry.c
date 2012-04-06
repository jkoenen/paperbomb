#include "geometry.h"
#include "vector.h"

int isCircleCircleIntersecting( const Circle* pCircleA, const Circle* pCircleB )
{
	return float2_squareDistance( &pCircleA->center, &pCircleB->center ) < float_sqr( pCircleA->radius + pCircleB->radius );
}

static float getLinePointDistance( const Line* pLine, const float2* pPoint )
{
	float2 ab;
	float2_sub( &ab, &pLine->b, &pLine->a );

	float2 ap;
	float2_sub( &ap, pPoint, &pLine->a );

	const float v = ( ap.x * ab.x + ap.y * ab.y ) / ( ab.x * ab.x + ab.y * ab.y );

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
	return getLinePointDistance( pLine, &pCircle->center ) <= pCircle->radius;
}

int isCircleCapsuleIntersecting( const Circle* pCircle, const Capsule* pCapsule )
{
	return getLinePointDistance( &pCapsule->line, &pCircle->center ) <= pCircle->radius + pCapsule->radius;
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

