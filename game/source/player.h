#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "types.h"

typedef struct 
{
	float2	position;
	float	direction;
	float	steer;
	float2	velocity;
	uint	health;
	uint32	lastInputMask;

} Player;

void player_reset( Player* pPlayer );
void player_update( Player* pPlayer, float timeStep, uint32 inputMask );

#endif
