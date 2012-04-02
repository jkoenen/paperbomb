#include "geometry.h"
#include "vector.h"

int isCircleCircleIntersecting( const Circle* pCircleA, const Circle* pCircleB )
{
	return float2_squareDistance( &pCircleA->center, &pCircleB->center ) < float_sqr( pCircleA->radius + pCircleB->radius );
}

int isCircleLineIntersecting( const Circle* pCircle, const Line* pLine )
{
	(void*)pCircle;
	(void*)pLine;
	return 0;
}

int isCircleCapsuleIntersecting( const Circle* pCircle, const Capsule* pCapsule )
{
	(void*)pCircle;
	(void*)pCapsule;
	return 0;
}