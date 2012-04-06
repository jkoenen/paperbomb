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

static void bomb_explode( ServerExplosion* pExplosion, const ServerBomb* pBomb )
{
	pExplosion->position	= pBomb->position;
	pExplosion->direction	= pBomb->direction;
	pExplosion->length		= pBomb->length;
	pExplosion->time		= GAMETIMESTEP;
}

static void bomb_update( ServerBomb* pBomb, ServerExplosion* pExplosion )
{
	pBomb->time += GAMETIMESTEP;
	if( pBomb->time >= bombTime )
	{
		if( pExplosion )
		{
			bomb_explode( pExplosion, pBomb );
		}

		pBomb->time = 0.0f;
	}
}

static void bomb_place( ServerBomb* pBomb, uint player, const float2* pPosition, float direction, float length )
{
	pBomb->player		= player;
	pBomb->position		= *pPosition;
	pBomb->direction	= direction;
	pBomb->length		= length;
	pBomb->time			= GAMETIMESTEP;
}

static ServerBomb* find_free_bomb( ServerBomb* pBombs, uint size )
{
	for( uint i = 0u; i < size; ++i )
	{
		if( pBombs[ i ].time == 0.0f )
		{
			return &pBombs[ i ];
		}
	}
	return 0;
}

static ServerExplosion* find_free_explosion( ServerExplosion* pExplosions, uint size )
{
	for( uint i = 0u; i < size; ++i )
	{
		if( pExplosions[ i ].time == 0.0f )
		{
			return &pExplosions[ i ];
		}
	}
	return 0;
}

static void player_init( ServerPlayer* pPlayer, const IP4Address* pAddress )
{
	pPlayer->playerState	= PlayerState_Active;

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

static void player_update( ServerPlayer* pPlayer, uint index, ServerBomb* pBomb, uint activeBombs )
{
	const float steerSpeed		= 0.05f;
	const float steerDamping	= 0.8f;
	const float maxSpeed		= 0.5f;
	const float maxSteer		= (float)PI * 0.2f;

	const uint buttonMask		= pPlayer->state.buttonMask;
	const uint buttonDownMask	= buttonMask & ~pPlayer->lastButtonMask;
	pPlayer->lastButtonMask		= buttonMask; 

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
}

static void create_client_state( ClientGameState* pClientState, const ServerGameState* pServerState )
{
	pClientState->id = pServerState->id;

	for( uint i = 0u; i < SYS_COUNTOF( pServerState->player ); ++i )
	{
		ClientPlayer* pClient = &pClientState->player[ i ];
		const ServerPlayer* pServer = &pServerState->player[ i ];

		pClient->state		= (uint8)pServer->playerState;
		copyString( pClient->name, sizeof( pClient->name ), pServer->name );
		pClient->posX		= float_quantize( pServer->position.x );
		pClient->posY		= float_quantize( pServer->position.y );
		pClient->direction	= angle_quantize( pServer->direction );
		pClient->age		= time_quantize( pServer->age );
		pClient->steer		= angle_quantize( pServer->steer );
	}

	for( uint i = 0u; i < SYS_COUNTOF( pServerState->bombs ); ++i )
	{
		ClientBomb* pClient = &pClientState->bombs[ i ];
		const ServerBomb* pServer = &pServerState->bombs[ i ];

		pClient->time		= time8_quantize( pServer->time );
		pClient->posX		= float_quantize( pServer->position.x );
		pClient->posY		= float_quantize( pServer->position.y );
		pClient->direction	= angle_quantize( pServer->direction );
		pClient->length		= (uint8)pServer->length;
	}

	for( uint i	= 0u; i < SYS_COUNTOF( pServerState->explosions ); ++i )
	{
		ClientExplosion* pClient = &pClientState->explosions[ i ];
		const ServerExplosion* pServer = &pServerState->explosions[ i ];

		pClient->time		= time8_quantize( pServer->time );
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

	for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.player ); ++i )
	{
		 pServer->gameState.player[ i ].playerState = PlayerState_InActive;
	}
	for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.bombs ); ++i )
	{
		pServer->gameState.bombs[ i ].time = 0.0f;
	}
	for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.explosions ); ++i )
	{
		pServer->gameState.explosions[ i ].time = 0.0f;
	}
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
			int freeIndex = -1;
			for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.player ); ++i )
			{
				if( pServer->gameState.player[ i ].playerState == PlayerState_InActive )
				{
					if( freeIndex < 0 )
					{
						freeIndex = (int)i;
					}
					continue;
				}

				if( socket_isAddressEqual( &pServer->gameState.player[ i ].address, &from ) )
				{
					if( isOnline )
					{
						pPlayer = &pServer->gameState.player[ i ];
					}
					else
					{
						for( uint j = 0u; j < SYS_COUNTOF( pServer->gameState.bombs ); ++j )
						{
							ServerBomb* pBomb = &pServer->gameState.bombs[ j ];
							if( ( pBomb->time > 0.0f ) && ( pBomb->player == i ) )
							{
								pBomb->player = MaxPlayer;
							}
						}

						pServer->gameState.player[ i ].playerState = PlayerState_InActive;
					}
					break;
				}
			}

			if( ( pPlayer == 0 ) && ( freeIndex >= 0 ) && isOnline )
			{
				pPlayer = &pServer->gameState.player[ freeIndex ];

				player_init( pPlayer, &from );
				player_respawn( pPlayer, &s_playerStartPositions[ freeIndex ], s_playerStartDirections[ freeIndex ] );
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

	for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.player ); ++i )
	{
		ServerPlayer* pPlayer = &pServer->gameState.player[ i ];
		if( pPlayer->playerState == PlayerState_InActive )
		{
			continue;
		}

		uint bombCount = 0u;
		for( uint j = 0u; j < SYS_COUNTOF( pServer->gameState.bombs ); ++j )
		{
			ServerBomb* pBomb = &pServer->gameState.bombs[ j ];
			if( ( pBomb->time > 0.0f ) && ( pBomb->player == i ) )
			{
				bombCount++;
			}
		}

		ServerBomb* pBomb = find_free_bomb( pServer->gameState.bombs, SYS_COUNTOF( pServer->gameState.bombs ) );
		player_update( pPlayer, i, pBomb, bombCount );
	}

	for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.bombs ); ++i )
	{
		ServerBomb* pBomb = &pServer->gameState.bombs[ i ];
		if( pBomb->time == 0.0f )
		{
			continue;
		}

		ServerExplosion* pExplosion = find_free_explosion( pServer->gameState.explosions, SYS_COUNTOF( pServer->gameState.explosions ) );
		bomb_update( pBomb, pExplosion );
	}

	for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.explosions ); ++i )
	{
		ServerExplosion* pExplosion = &pServer->gameState.explosions[ i ];
		if( pExplosion->time == 0.0f )
		{
			continue;
		}

		if( pExplosion->time == GAMETIMESTEP )
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

			for( uint j = 0u; j < SYS_COUNTOF( pServer->gameState.player ); ++j )
			{
				ServerPlayer* pPlayer = &pServer->gameState.player[ j ];
				if( pPlayer->playerState == PlayerState_InActive )
				{
					continue;
				}

				Circle playerCirlce;
				playerCirlce.center = pPlayer->position;
				playerCirlce.radius = 1.0f;

				const int isPlayerOldEnough = pPlayer->age > s_playerBulletProofAge;

				if( isPlayerOldEnough && ( isCircleCapsuleIntersecting( &playerCirlce, &capsule0 ) || isCircleCapsuleIntersecting( &playerCirlce, &capsule1 ) ) )
				{
					player_respawn( pPlayer, &s_playerStartPositions[ i ], s_playerStartDirections[ i ] );
				}
			}

			for( uint j = 0u; j < SYS_COUNTOF( pServer->gameState.bombs ); ++j )
			{
				ServerBomb* pBomb = &pServer->gameState.bombs[ j ];
				if( pBomb->time == 0.0f )
				{
					continue;
				}

				Circle bombCircle;
				bombCircle.center = pBomb->position;
				bombCircle.radius = 1.0f;

				if( isCircleCapsuleIntersecting( &bombCircle, &capsule0 ) || isCircleCapsuleIntersecting( &bombCircle, &capsule1 ) )
				{
					ServerExplosion* pExplosion = find_free_explosion( pServer->gameState.explosions, SYS_COUNTOF( pServer->gameState.explosions ) );
					bomb_explode( pExplosion, pBomb );

					pBomb->time = 0.0f;
				}
			}
		}

		pExplosion->time += GAMETIMESTEP;
		if( pExplosion->time >= explosionTime )
		{
			pExplosion->time = 0.0f;
		}
	}

	pServer->gameState.id++;

	ClientGameState clientState;
	create_client_state( &clientState, &pServer->gameState );

	//SYS_TRACE_DEBUG( "### %d\n", sizeof( clientState ) );

	for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.player ); ++i )
	{
		ServerPlayer* pPlayer = &pServer->gameState.player[ i ];
		if( pPlayer->playerState == PlayerState_InActive )
		{
			continue;
		}

		socket_send_blocking( pServer->socket, &pPlayer->address, &clientState, sizeof( clientState ) );
	}
}
