#ifndef BOMB_H_INCLUDED
#define BOMB_H_INCLUDED

#include "types.h"

typedef struct 
{
	int		active;
	float2	position;
	float	direction;
	float	length;
	float	time;

} Bomb;

typedef struct 
{
	float2	position;
	float	direction;
	float	length;
	float	time;

} Explosion;

uint	bomb_update( Bomb* pBomb, Explosion* pExplosion );
void	bomb_explode( Explosion* pExplosion, const Bomb* pBomb );

#endif

