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

int		isCircleCircleIntersecting( const Circle* pCircleA, const Circle* pCircleB );
int		isCircleLineIntersecting( const Circle* pCircle, const Line* pLine );
int		isCircleCapsuleIntersecting( const Circle* pCircle, const Capsule* pCapsule );

#endif

