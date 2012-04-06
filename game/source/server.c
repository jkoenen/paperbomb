#include "server.h"

#include "input.h"
#include "vector.h"
#include "matrix.h"
#include "geometry.h"
#include "debug.h"

static const float s_playerBulletProofAge = 1.0f;

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

void bomb_explode( ServerExplosion* pExplosion, const ServerBomb* pBomb )
{
	pExplosion->position	= pBomb->position;
	pExplosion->direction	= pBomb->direction;
	pExplosion->length		= pBomb->length;
	pExplosion->time		= 0.0f;
}

uint bomb_update( ServerBomb* pBomb, ServerExplosion* pExplosion )
{
	uint explosions = 0u;

	pBomb->time -= GAMETIMESTEP;
	if( pBomb->time <= 0.0f )
	{
		if( pExplosion )
		{
			bomb_explode( pExplosion, pBomb );
			explosions = 1u;
		}
	}

	return explosions;
}

void bomb_place( ServerBomb* pBomb, uint player, const float2* pPosition, float direction, float length )
{
	pBomb->player		= player;
	pBomb->position		= *pPosition;
	pBomb->direction	= direction;
	pBomb->length		= length;
	pBomb->time			= 2.0f;
}

static void player_init( ServerPlayer* pPlayer, const IP4Address* pAddress )
{
	pPlayer->address		= *pAddress;
	pPlayer->lastButtonMask	= 0u;
	pPlayer->state.id		= 0u;

	pPlayer->maxBombs		= 4u;
	pPlayer->bombLength		= 12.0f;
}

static void player_respawn( ServerPlayer* pPlayer, const float2* pPosition, float direction )
{
	pPlayer->age		= 0.0f;
	pPlayer->steer		= 0.0f;
	pPlayer->velocity.x	= 0.0f;
	pPlayer->position.y	= 0.0f;
	pPlayer->position	= *pPosition;
	pPlayer->direction	= direction;
}

static uint player_update( ServerPlayer* pPlayer, uint index, ServerBomb* pBomb, uint activeBombs )
{
	const float steerSpeed		= 0.05f;
	const float steerDamping	= 0.8f;
	const float maxSpeed		= 0.5f;
	const float maxSteer		= (float)PI * 0.2f;

	const uint buttonMask		= pPlayer->state.buttonMask;
	const uint buttonDownMask	= buttonMask & ~pPlayer->lastButtonMask;
	pPlayer->lastButtonMask		= buttonMask; 

	uint bombs = 0u;

	pPlayer->age += GAMETIMESTEP;

	if( buttonMask & ButtonMask_Left )
	{
		pPlayer->steer = float_min( pPlayer->steer + steerSpeed, maxSteer );
	}
	else if( buttonMask & ButtonMask_Right )
	{
		pPlayer->steer = float_max( pPlayer->steer - steerSpeed, -maxSteer );
	}
	else
	{
		pPlayer->steer *= steerDamping;
	}

	float acceleration = 0.0f;
	if( buttonMask & ButtonMask_Up )
	{
		acceleration = 0.02f;
	}
	else if( buttonMask & ButtonMask_Down )
	{ 
		acceleration = -0.02f;
	}

	if( buttonDownMask & ButtonMask_PlaceBomb )
	{
		if( ( pPlayer->maxBombs > activeBombs ) && pBomb )
		{
			bomb_place( pBomb, index, &pPlayer->position, pPlayer->direction, pPlayer->bombLength );
			bombs = 1u;
		}
	}

	float2 directionVector;
	float2_from_angle( &directionVector, pPlayer->direction );

	float dirVecDot = float2_dot( &directionVector, &pPlayer->velocity );

	pPlayer->direction += dirVecDot * pPlayer->steer;

	float2 velocityNormalized = pPlayer->velocity;
	float2_normalize0( &velocityNormalized );
	dirVecDot = float2_dot( &directionVector, &velocityNormalized );

	float2 velocityForward = pPlayer->velocity;
	float2_scale1f( &velocityForward, &velocityForward, float_abs( dirVecDot ) );

	float2 velocitySide;
	float2_sub( &velocitySide, &pPlayer->velocity, &velocityForward );

	float2 velocity;
	velocity.x = acceleration;
	velocity.y = 0.0f;

	float2_rotate( &velocity, pPlayer->direction );
	float2_addScaled1f( &velocity, &velocity, &velocityForward, 0.94f );
	float2_addScaled1f( &velocity, &velocity, &velocitySide, 0.8f );

	pPlayer->velocity = velocity; 

	const float speed = float_abs( float2_length( &pPlayer->velocity ) );
	if( speed > maxSpeed )
	{
		float2_scale1f( &pPlayer->velocity, &pPlayer->velocity, maxSpeed / speed );
	}

	//	SYS_TRACE_DEBUG( "speed %.4f\n", speed );

	float2_add( &pPlayer->position, &pPlayer->position, &pPlayer->velocity );

	pPlayer->position.x = float_clamp( pPlayer->position.x, -32.0f, 32.0f );
	pPlayer->position.y = float_clamp( pPlayer->position.y, -18.0f, 18.0f );

	return bombs;
}

static void create_client_state( ClientGameState* pClientState, const ServerGameState* pServerState )
{
	pClientState->id = pServerState->id;

	pClientState->playerCount = (uint8)pServerState->playerCount;
	for( uint i = 0u; i < pServerState->playerCount; ++i )
	{
		ClientPlayer* pClient = &pClientState->player[ i ];
		const ServerPlayer* pServer = &pServerState->player[ i ];

		copyString( pClient->name, sizeof( pClient->name ), pServer->name );
		pClient->posX		= float_quantize( pServer->position.x );
		pClient->posY		= float_quantize( pServer->position.y );
		pClient->direction	= angle_quantize( pServer->direction );
		pClient->age		= time_quantize( pServer->age );
		pClient->steer		= angle_quantize( pServer->steer );
	}

	pClientState->bombCount = (uint8)pServerState->bombCount;
	for( uint i = 0u; i < pServerState->bombCount; ++i )
	{
		ClientBomb* pClient = &pClientState->bombs[ i ];
		const ServerBomb* pServer = &pServerState->bombs[ i ];

		pClient->posX		= float_quantize( pServer->position.x );
		pClient->posY		= float_quantize( pServer->position.y );
		pClient->direction	= angle_quantize( pServer->direction );
		pClient->length		= (uint8)pServer->length;
	}

	pClientState->explosionCount = (uint8)pServerState->explosionCount;
	for( uint i = 0u; i < pServerState->explosionCount; ++i )
	{
		ClientExplosion* pClient = &pClientState->explosions[ i ];
		const ServerExplosion* pServer = &pServerState->explosions[ i ];

		pClient->posX		= float_quantize( pServer->position.x );
		pClient->posY		= float_quantize( pServer->position.y );
		pClient->direction	= angle_quantize( pServer->direction );
		pClient->length		= (uint8)pServer->length;
	}
}

void server_create( Server* pServer, uint16 port )
{
	socket_init();

	pServer->socket = socket_create();

	IP4Address address;
	address.address = socket_gethostIP();
	address.port	= port;

	socket_bind( pServer->socket, &address );

	pServer->gameState.playerCount = 0u;
	pServer->gameState.explosionCount = 0u;
	pServer->gameState.id = 0u;
}

void server_destroy( Server* pServer )
{
	socket_destroy( pServer->socket );
	pServer->socket = InvalidSocket;

	socket_done();
}

void server_update( Server* pServer, const World* pWorld )
{
	(void)pWorld;

	for(;;)
	{
		ClientState state;
		IP4Address from;
		const int result = socket_receive( pServer->socket, &state, sizeof( state ), &from );
		if( result > 0 )
		{
			SYS_ASSERT( result == sizeof( state ) );
			//SYS_TRACE_DEBUG( "s recv %d\n", state.id );

			const int isOnline = ( state.flags & ClientStateFlag_Online );

			ServerPlayer* pPlayer = 0;
			for( uint i = 0u; i < pServer->gameState.playerCount; ++i )
			{
				if( socket_isAddressEqual( &pServer->gameState.player[ i ].address, &from ) )
				{
					if( isOnline )
					{
						pPlayer = &pServer->gameState.player[ i ];
					}
					else
					{
						for( uint j = 0u; j < pServer->gameState.bombCount; ++j )
						{
							ServerBomb* pBomb = &pServer->gameState.bombs[ j ];
							if( pBomb->player == i )
							{
								pBomb->player = MaxPlayer;
							}
						}

						if( i + 1u < pServer->gameState.playerCount )
						{
							pServer->gameState.player[ i ] = pServer->gameState.player[ pServer->gameState.playerCount - 1u ];
						}
						pServer->gameState.playerCount--;
					}
					break;
				}
			}
			if( ( pPlayer == 0 ) && ( pServer->gameState.playerCount < SYS_COUNTOF( pServer->gameState.player ) ) && isOnline )
			{
				const uint index = pServer->gameState.playerCount;

				pPlayer = &pServer->gameState.player[ index ];
				player_init( pPlayer, &from );
				player_respawn( pPlayer, &s_playerStartPositions[ index ], s_playerStartDirections[ index ] );

				pServer->gameState.playerCount++;
			}

			if( pPlayer != 0 )
			{
				if( state.id > pPlayer->state.id )
				{
					pPlayer->state = state;
				}
			}
		}
		else
		{
			break;
		}
	}

	for( uint i = 0u; i < pServer->gameState.playerCount; ++i )
	{
		ServerPlayer* pPlayer = &pServer->gameState.player[ i ];

		uint bombCount = 0u;
		for( uint j = 0u; j < pServer->gameState.bombCount; ++j )
		{
			ServerBomb* pBomb = &pServer->gameState.bombs[ j ];
			if( pBomb->player == i )
			{
				bombCount++;
			}
		}

		ServerBomb* pBomb = ( pServer->gameState.bombCount < SYS_COUNTOF( pServer->gameState.bombs ) ? &pServer->gameState.bombs[ pServer->gameState.bombCount ] : 0 );
		pServer->gameState.bombCount += player_update( pPlayer, i, pBomb, bombCount );
	}

	{
		uint bombIndex = 0u;
		while( bombIndex < pServer->gameState.bombCount )
		{
			ServerBomb* pBomb = &pServer->gameState.bombs[ bombIndex ];

			ServerExplosion* pExplosion = ( pServer->gameState.explosionCount < SYS_COUNTOF( pServer->gameState.explosions ) ? &pServer->gameState.explosions[ pServer->gameState.explosionCount ] : 0 );
			if( bomb_update( pBomb, pExplosion ) )
			{
				if( pExplosion )
				{
					pServer->gameState.explosionCount++;
				}

				if( bombIndex + 1u < pServer->gameState.bombCount )
				{
					*pBomb = pServer->gameState.bombs[ pServer->gameState.bombCount - 1u ];
				}
				pServer->gameState.bombCount--;
			}
			else
			{
				bombIndex++;
			}
		}
	}

	uint explosionIndex = 0u;
	while( explosionIndex < pServer->gameState.explosionCount )
	{
		ServerExplosion* pExplosion = &pServer->gameState.explosions[ explosionIndex ];

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

			float2_set( &length, 0.0f, pExplosion->length );
			float2_rotate( &length, pExplosion->direction );

			float2_add( &capsule1.line.a, &pExplosion->position, &length );
			float2_sub( &capsule1.line.b, &pExplosion->position, &length );

			for( uint i = 0u; i < pServer->gameState.playerCount; ++i )
			{
				ServerPlayer* pPlayer = &pServer->gameState.player[ i ];

				Circle playerCirlce;
				playerCirlce.center = pPlayer->position;
				playerCirlce.radius = 1.0f;

				const int isPlayerOldEnough = pPlayer->age > s_playerBulletProofAge;

				if( isPlayerOldEnough && ( isCircleCapsuleIntersecting( &playerCirlce, &capsule0 ) || isCircleCapsuleIntersecting( &playerCirlce, &capsule1 ) ) )
				{
					player_respawn( pPlayer, &s_playerStartPositions[ i ], s_playerStartDirections[ i ] );
				}
			}

			uint bombIndex = 0u;
			while( bombIndex < pServer->gameState.bombCount )
			{
				ServerBomb* pBomb = &pServer->gameState.bombs[ bombIndex ];

				Circle bombCircle;
				bombCircle.center = pBomb->position;
				bombCircle.radius = 1.0f;

				if( isCircleCapsuleIntersecting( &bombCircle, &capsule0 ) || isCircleCapsuleIntersecting( &bombCircle, &capsule1 ) )
				{
					if( pServer->gameState.explosionCount < SYS_COUNTOF( pServer->gameState.explosions ) )
					{
						bomb_explode( &pServer->gameState.explosions[ pServer->gameState.explosionCount++ ], pBomb );
					}

					if( bombIndex + 1u < pServer->gameState.bombCount )
					{
						*pBomb = pServer->gameState.bombs[ pServer->gameState.bombCount - 1u ];
					}
					pServer->gameState.bombCount--;
				}
				else
				{
					bombIndex++;
				}
			}
		}

		pExplosion->time += GAMETIMESTEP;
		if( pExplosion->time > 1.0f )
		{
			if( explosionIndex + 1u < pServer->gameState.explosionCount )
			{
				*pExplosion = pServer->gameState.explosions[ pServer->gameState.explosionCount - 1u ];
			}
			pServer->gameState.explosionCount--;
		}
		else
		{
			explosionIndex++;
		}
	}

	pServer->gameState.id++;

	ClientGameState clientState;
	create_client_state( &clientState, &pServer->gameState );

	//SYS_TRACE_DEBUG( "### %d\n", sizeof( clientState ) );

	for( uint i = 0u; i < pServer->gameState.playerCount; ++i )
	{
		socket_send_blocking( pServer->socket, &pServer->gameState.player[ i ].address, &clientState, sizeof( clientState ) );
	}
}
