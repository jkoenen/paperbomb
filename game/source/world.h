#ifndef WORLD_H_INCLUDED
#define WORLD_H_INCLUDED

#include "geometry.h"
#include "matrix.h"

typedef struct 
{
	float2x3	worldTransform;

	float2		borderMin;
	float2		borderMax;

	Circle		rockz[ 4u ];

} World;

#endif
