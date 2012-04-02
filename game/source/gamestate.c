#include "gamestate.h"

#include "player.h"
#include "vector.h"
#include "geometry.h"

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
	pGameState->explosionCount = 0u;
}

void gamestate_update( GameState* pGameState, const World* pWorld, const uint32* pInputs )
{
	SYS_USE_ARGUMENT( pWorld );
	for( uint i = 0u; i < pGameState->playerCount; ++i )
	{
		Player* pPlayer = &pGameState->player[ i ];
		player_update_input( pPlayer, pInputs[ i ] );

		for( uint j = 0u; j < SYS_COUNTOF( pPlayer->bombs ); ++j )
		{
			Bomb* pBomb = &pPlayer->bombs[ j ];

			bomb_update( pBomb, &pGameState->explosions[ pGameState->explosionCount ], SYS_COUNTOF( pGameState->explosions ) - pGameState->explosionCount );
		}
	}

	uint index = 0u;
	while( index < pGameState->explosionCount )
	{
		Explosion* pExplosion = &pGameState->explosions[ index ];

		pExplosion->time += GAMETIMESTEP;
		if( pExplosion->time > 1.0f )
		{
			pGameState->explosionCount--;
			*pExplosion = pGameState->explosions[ pGameState->explosionCount ];
			continue;
		}

		float2 length;
		float2_set( &length, pExplosion->length, 0.0f );
		float2_rotate( &length, pExplosion->direction );

		Capsule capsule;
		capsule.radius = 1.0f;
		float2_add( &capsule.line.a, &pExplosion->positon, &length );
		float2_sub( &capsule.line.b, &pExplosion->positon, &length );

		//for( uint i = 0u; i < pGameState->playerCount; ++i )
		//{
		//	Player* pPlayer = &pGameState->player[ i ];

		//	Circle playerCirlce;
		//	playerCirlce.center = pPlayer->position;
		//	playerCirlce.radius = 1.0f;

		//	if( isCircleCapsuleIntersecting( &playerCirlce, &capsule ) )
		//	{
		//		
		//	}

		//	for( uint j = 0u; j < SYS_COUNTOF( pPlayer->bombs ); ++j )
		//	{
		//		Bomb* pBomb = &pPlayer->bombs[ j ];

		//if( isCircleCapsuleLIntersecting())

		index++;
	}
}

