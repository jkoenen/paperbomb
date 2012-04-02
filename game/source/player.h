#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "types.h"
#include "bomb.h"

enum
{
	MaxBombs = 4u
};

typedef struct 
{
	float2	position;
	float	direction;
	float	steer;
	float2	velocity;
	float	health;
	uint32	lastButtonMask;
	uint	maxBombs;
	Bomb	bombs[ MaxBombs ];

} Player;

void player_reset( Player* pPlayer );
void player_update( Player* pPlayer, uint32 buttonMask );

#endif
