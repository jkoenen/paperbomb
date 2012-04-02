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

			pGameState->explosionCount += bomb_update( pBomb, SYS_COUNTOF( pGameState->explosions ) > pGameState->explosionCount ? &pGameState->explosions[ pGameState->explosionCount ] : 0 );
		}
	}

	uint index = 0u;
	while( index < pGameState->explosionCount )
	{
		Explosion* pExplosion = &pGameState->explosions[ index ];

		if( pExplosion->time == 0.0f )
		{
			Capsule capsule0;
			Capsule capsule1;
			capsule0.radius = 1.0f;
			capsule1.radius = 1.0f;

			float2 length;
			float2_set( &length, pExplosion->length, 0.0f );
			float2_rotate( &length, pExplosion->direction );

			float2_add( &capsule0.line.a, &pExplosion->position, &length );
			float2_sub( &capsule0.line.b, &pExplosion->position, &length );

			float2_set( &length, pExplosion->length, 0.0f );
			float2_rotate( &length, pExplosion->direction + (float)PI );

			float2_add( &capsule1.line.a, &pExplosion->position, &length );
			float2_sub( &capsule1.line.b, &pExplosion->position, &length );

			for( uint i = 0u; i < pGameState->playerCount; ++i )
			{
				Player* pPlayer = &pGameState->player[ i ];

				Circle playerCirlce;
				playerCirlce.center = pPlayer->position;
				playerCirlce.radius = 1.0f;

				for( uint j = 0u; j < SYS_COUNTOF( pPlayer->bombs ); ++j )
				{
					Bomb* pBomb = &pPlayer->bombs[ j ];

					if( pBomb->active )
					{
						Circle bombCircle;
						bombCircle.center = pBomb->position;
						bombCircle.radius = 1.0f;

						if( isCircleCapsuleIntersecting( &bombCircle, &capsule0 ) || isCircleCapsuleIntersecting( &bombCircle, &capsule1 ) )
						{
							if( pGameState->explosionCount < SYS_COUNTOF( pGameState->explosions ) )
							{
								bomb_explode( &pGameState->explosions[ pGameState->explosionCount++ ], pBomb );
								pBomb->active = 0;
							}
						}
					}
				}

				if( isCircleCapsuleIntersecting( &playerCirlce, &capsule0 ) || isCircleCapsuleIntersecting( &playerCirlce, &capsule1 ) )
				{
					player_init( pPlayer, &s_playerStartPositions[ i ], s_playerStartDirections[ i ] );
				}
			}
		}

		pExplosion->time += GAMETIMESTEP;
		if( pExplosion->time > 1.0f )
		{
			pGameState->explosionCount--;
			if( pGameState->explosionCount > 0u )
			{
				*pExplosion = pGameState->explosions[ pGameState->explosionCount ];
			}
			continue;
		}

		index++;
	}
}

