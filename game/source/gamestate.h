#ifndef GAMESTATE_H_INCLUDED
#define GAMESTATE_H_INCLUDED

#include "types.h"
#include "player.h"
#include "bomb.h"
#include "world.h"

enum
{
	MaxPlayer		= 4u,
	MaxExplosions	= 32u
};

typedef struct 
{
	Player		player[ MaxPlayer ];
	uint		playerCount;

	Explosion	explosions[ MaxExplosions ];
	uint		explosionCount;

} GameState;

void gamestate_init( GameState* pGameState, uint playerCount );
void gamestate_update( GameState* pGameState, const World* pWorld, const uint32* pInputs );

#endif
