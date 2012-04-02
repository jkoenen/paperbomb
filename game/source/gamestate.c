#include "gamestate.h"

#include <math.h>

static const float2 s_playerStartPositions[] =
{
	{  4.0f,  4.0f },
	{ -4.0f,  4.0f },
	{ -4.0f, -4.0f },
	{  4.0f, -4.0f }
};

static const float s_playerStartDirections[] =
{
	(float)PI * 0.25f * 1.0f,
	(float)PI * 0.25f * 3.0f,
	(float)PI * 0.25f * 5.0f,
	(float)PI * 0.25f * 7.0f
};

void gamestate_init( GameState* pGameState, uint playerCount )
{
	pGameState->playerCount = playerCount;
	for( uint i = 0u; i < pGameState->playerCount; ++i )
	{
		player_init( &pGameState->player[ i ], &s_playerStartPositions[ i ], s_playerStartDirections[ i ] );
	}
}

void gamestate_update( GameState* pGameState, const World* pWorld, const uint32* pInputs )
{
	(void*)pWorld;
	for( uint i = 0u; i < pGameState->playerCount; ++i )
	{
		player_update( &pGameState->player[ i ], pInputs[ i ] );
	}
}