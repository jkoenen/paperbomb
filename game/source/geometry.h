#ifndef GEOMETRY_H_INCLUDE
#define GEOMETRY_H_INCLUDE

#include "types.h"

typedef struct 
{
	float2	a;
	float2	b;
} Line;

typedef struct  
{
	Line	line;
	float	radius;
} Capsule;

typedef struct 
{
	float2	center;
	float	radius;
} Circle;

int		isCircleCircleIntersectingWithDistance( const Circle* pCircle, const Line* pLine, float* pDistance );
int		isLineLineIntersectingWithDistance( const Line* pLineA, const Line* pLineB, float* pDistance );

int		isCircleCircleIntersecting( const Circle* pCircleA, const Circle* pCircleB );
int		isCircleLineIntersecting( const Circle* pCircle, const Line* pLine );
int		isCircleCapsuleIntersecting( const Circle* pCircle, const Capsule* pCapsule );

int		circleCircleCollide( const Circle* pFirst, const Circle* pSecond, float massRatio, float2* pFirstPos, float2* pSecondPos );

#endif

