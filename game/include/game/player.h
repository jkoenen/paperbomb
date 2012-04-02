#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "sys/types.h"

typedef struct 
{
	float2	position;
	float	direction;
	float	steer;
	float2	velocity;
	uint	health;
	uint32	lastInputMask;

} player_t;

void player_reset( player_t* pPlayer );
void player_update( player_t* pPlayer, float timeStep, uint32 inputMask );

#endif
