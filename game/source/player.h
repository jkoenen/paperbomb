#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "types.h"
#include "bomb.h"

enum
{
	MaxBombs = 8u
};

typedef struct 
{
	float	age;
	float2	position;
	float	direction;
	float	steer;
	float2	velocity;
	float	health;
	uint32	lastButtonMask;
	uint	maxBombs;
	float	bombLength;
	Bomb	bombs[ MaxBombs ];

} Player;

void player_init( Player* pPlayer, const float2* pPosition, float direction, int clearBombs );
void player_update_input( Player* pPlayer, uint32 buttonMask );

#endif
