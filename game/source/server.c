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

static void bomb_explode( ServerExplosion* pExplosion, const ServerBomb* pBomb, const World* pWorld )
{
	if( !pExplosion )
	{
		return;
	}

 	pExplosion->player		= pBomb->player;
	pExplosion->position	= pBomb->position;
	pExplosion->direction	= pBomb->direction;
	pExplosion->time		= GAMETIMESTEP;

	const float2 borderLines[] = 
	{
		{ pWorld->borderMin.x, pWorld->borderMin.y }, { pWorld->borderMax.x, pWorld->borderMin.y },
		{ pWorld->borderMax.x, pWorld->borderMin.y }, { pWorld->borderMax.x, pWorld->borderMax.y },
		{ pWorld->borderMax.x, pWorld->borderMax.y }, { pWorld->borderMin.x, pWorld->borderMax.y },
		{ pWorld->borderMin.x, pWorld->borderMax.y }, { pWorld->borderMin.x, pWorld->borderMin.y },
	};

	Line line;
	line.a = pExplosion->position;
	float direction = pExplosion->direction;
	for( uint i = 0u; i < 4u; ++i )
	{
		float2_set( &line.b, 1000.0f, 0.0f );
		float2_rotate( &line.b, direction );

		float minDistace = pBomb->length;
		for( uint j = 0u; j < SYS_COUNTOF( pWorld->rockz ); ++j )
		{
			float distance;
			if( isCircleCircleIntersectingWithDistance( &pWorld->rockz[ j ], &line, &distance ) )
			{
				minDistace = float_min( minDistace, distance );
			}
		}

		for( uint j = 0u; j < SYS_COUNTOF( borderLines ); j += 2u )
		{
			Line boderLine;
			boderLine.a = borderLines[ j ];
			boderLine.b = borderLines[ j + 1u ];

			float distance;
			if( isLineLineIntersectingWithDistance( &line, &boderLine, &distance ) )
			{
				minDistace = float_min( minDistace, distance );
			}
		}

		pExplosion->length[ i ] = minDistace;
		direction += HALFPI;
	}
}

static void bomb_update( ServerBomb* pBomb, ServerExplosion* pExplosion, const World* pWorld )
{
	pBomb->time += GAMETIMESTEP;
	if( pBomb->time >= s_bombTime )
	{
		if( pExplosion )
		{
			bomb_explode( pExplosion, pBomb, pWorld );
		}

		pBomb->time = 0.0f;
	}
}

static void bomb_place( ServerBomb* pBomb, uint player, const float2* pPosition, float direction, float length, const World* pWorld )
{
	float2 offset;
	float2_set( &offset, -s_bombCarOffset, 0.0f );
	float2_rotate( &offset, direction );

	pBomb->position = *pPosition;
	float2_add( &pBomb->position, &pBomb->position, &offset );

	pBomb->position.x = float_clamp( pBomb->position.x, pWorld->borderMin.x + s_bombRadius, pWorld->borderMax.x - s_bombRadius );
	pBomb->position.y = float_clamp( pBomb->position.y, pWorld->borderMin.y + s_bombRadius, pWorld->borderMax.y - s_bombRadius );

	pBomb->player		= player;
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
	pPlayer->frags			= 0u;
}

static void player_respawn( ServerPlayer* pPlayer, const float2* pPosition, float direction )
{
	pPlayer->age			= 0.0f;
	pPlayer->steer			= 0.0f;
	pPlayer->velocity.x		= 0.0f;
	pPlayer->position.y		= 0.0f;
	pPlayer->position		= *pPosition;
	pPlayer->direction		= direction;
	pPlayer->maxBombs		= StartBombs;
	pPlayer->bombLength		= s_startBombLength;
}

static void player_update( ServerPlayer* pPlayer, uint index, ServerBomb* pBomb, uint activeBombs, World* pWorld )
{
	const float steerSpeed		= 0.08f;
	const float steerDamping	= 0.8f;
	const float maxSpeed		= 0.3f;
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
		acceleration = 0.03f;
	}
	else if( buttonMask & ButtonMask_Down )
	{ 
		acceleration = -0.03f;
	}

	if( buttonDownMask & ButtonMask_PlaceBomb )
	{
		if( ( pPlayer->maxBombs > activeBombs ) && pBomb )
		{
			bomb_place( pBomb, index, &pPlayer->position, pPlayer->direction, pPlayer->bombLength, pWorld );
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
	float2_addScaled1f( &velocity, &velocity, &velocityForward, 0.96f );
	float2_addScaled1f( &velocity, &velocity, &velocitySide, 0.8f );

	pPlayer->velocity = velocity; 

	const float speed = float_abs( float2_length( &pPlayer->velocity ) );
	if( speed > maxSpeed )
	{
		float2_scale1f( &pPlayer->velocity, &pPlayer->velocity, maxSpeed / speed );
	}

	//	SYS_TRACE_DEBUG( "speed %.4f\n", speed );

	float2_add( &pPlayer->position, &pPlayer->position, &pPlayer->velocity );

	pPlayer->position.x = float_clamp( pPlayer->position.x, pWorld->borderMin.x + s_carRadius, pWorld->borderMax.x - s_carRadius );
	pPlayer->position.y = float_clamp( pPlayer->position.y, pWorld->borderMin.y + s_carRadius, pWorld->borderMax.y - s_carRadius );
}


static void create_client_state( ClientGameState* pClientState, const ServerGameState* pServerState )
{
	pClientState->id = pServerState->id;

	for( uint i = 0u; i < SYS_COUNTOF( pServerState->player ); ++i )
	{
		ClientPlayer* pClient = &pClientState->player[ i ];
		const ServerPlayer* pServer = &pServerState->player[ i ];

		pClient->state = (uint8)pServer->playerState;
		if( pServer->playerState != PlayerState_InActive )
		{
			copyString( pClient->name, sizeof( pClient->name ), pServer->name );
			pClient->posX		= float_quantize( pServer->position.x );
			pClient->posY		= float_quantize( pServer->position.y );
			pClient->direction	= angle_quantize( pServer->direction );
			pClient->age		= time_quantize( pServer->age );
			pClient->steer		= angle_quantize( pServer->steer );
			pClient->frags		= (int8)int_clamp( pServer->frags, -128, 127 );
		}
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

		pClient->time			= time8_quantize( pServer->time );
		pClient->posX			= float_quantize( pServer->position.x );
		pClient->posY			= float_quantize( pServer->position.y );
		pClient->direction		= angle_quantize( pServer->direction );
		pClient->length[ 0u ]	= (uint8)pServer->length[ 0u ];
		pClient->length[ 1u ]	= (uint8)pServer->length[ 1u ];
		pClient->length[ 2u ]	= (uint8)pServer->length[ 2u ];
		pClient->length[ 3u ]	= (uint8)pServer->length[ 3u ];
	}

	for( uint i	= 0u; i < SYS_COUNTOF( pServerState->items ); ++i )
	{
		ClientItem* pClient = &pClientState->items[ i ];
		const ServerItem* pServer = &pServerState->items[ i ];

		pClient->type		= (uint8)pServer->type;
		pClient->posX		= float_quantize( pServer->position.x );
		pClient->posY		= float_quantize( pServer->position.y );
	}
}

static void server_send_client_state( Server* pServer )
{
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

void server_create( Server* pServer, uint16 port )
{
	socket_init();

	pServer->socket = socket_create();

	IP4Address address;
	address.address = socket_getAnyIP();
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
	for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.items ); ++i )
	{
		pServer->gameState.items[ i ].type = ItemType_None;
	}
	pServer->gameState.timeToNextItem = s_itemMaxTime;

	pServer->gameState.id = 0u;
}

void server_destroy( Server* pServer )
{
	pServer->gameState.id |= ServerFlagOffline;
	server_send_client_state( pServer );

	socket_destroy( pServer->socket );
	pServer->socket = InvalidSocket;

	socket_done();
}

static int server_findFreePosition( float2* pPosition, const World* pWorld )
{
	for( uint i = 0u; i < 100u; ++i )
	{
		pPosition->x = float_rand_range( pWorld->borderMin.x + 2.0f, pWorld->borderMax.x - 2.0f );
		pPosition->y = float_rand_range( pWorld->borderMin.y + 2.0f, pWorld->borderMax.y - 2.0f );

		Circle circle;
		circle.center = *pPosition;
		circle.radius = s_itemRadius;

		int intersect = FALSE;
		for( uint j = 0u; j < SYS_COUNTOF( pWorld->rockz ); ++j )
		{
			if( isCircleCircleIntersecting( &pWorld->rockz[ j ], &circle ) )
			{
				intersect = TRUE;
				break;
			}
		}

		if( !intersect )
		{
			return TRUE;
		}
	}

	return FALSE;
}

void server_update( Server* pServer, World* pWorld )
{
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

						for( uint j = 0u; j < SYS_COUNTOF( pServer->gameState.explosions ); ++j )
						{
							ServerExplosion* pExplosion = &pServer->gameState.explosions[ j ];
							if( ( pExplosion->time > 0.0f ) && ( pExplosion->player == i ) )
							{
								pExplosion->player = MaxPlayer;
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

			if( pPlayer )
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
			const ServerBomb* pBomb = &pServer->gameState.bombs[ j ];
			if( ( pBomb->time > 0.0f ) && ( pBomb->player == i ) )
			{
				bombCount++;
			}
		}

		ServerBomb* pBomb = find_free_bomb( pServer->gameState.bombs, SYS_COUNTOF( pServer->gameState.bombs ) );
		player_update( pPlayer, i, pBomb, bombCount, pWorld );

		Circle playerCirlce;
		playerCirlce.center = pPlayer->position;
		playerCirlce.radius = s_carRadius;

		for( uint j = 0u; j < SYS_COUNTOF( pServer->gameState.items ); ++j )
		{
			ServerItem* pItem = &pServer->gameState.items[ j ];
			if( pItem->type == ItemType_None )
			{
				continue;
			}

			Circle itemCircle;
			itemCircle.center = pItem->position;
			itemCircle.radius = s_itemRadius;

			if( isCircleCircleIntersecting( &itemCircle, &playerCirlce ) )
			{
				switch( pItem->type )
				{
					case ItemType_BombRange:
						pPlayer->bombLength += s_itemBombLength;
						break;

					case ItemType_ExtraBomb:
						pPlayer->maxBombs++;
						break;
				}

				pItem->type = ItemType_None;
			}
		}
	}

	for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.bombs ); ++i )
	{
		ServerBomb* pBomb = &pServer->gameState.bombs[ i ];
		if( pBomb->time == 0.0f )
		{
			continue;
		}

		ServerExplosion* pExplosion = find_free_explosion( pServer->gameState.explosions, SYS_COUNTOF( pServer->gameState.explosions ) );
		bomb_update( pBomb, pExplosion, pWorld );
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
			capsule0.radius = s_explosionRadius;
			capsule1.radius = s_explosionRadius;

			float2 length0;
			float2_set( &length0, pExplosion->length[ 0u ], 0.0f );
			float2_rotate( &length0, pExplosion->direction );

			float2 length1;
			float2_set( &length1, pExplosion->length[ 2u ], 0.0f );
			float2_rotate( &length1, pExplosion->direction );

			float2_add( &capsule0.line.a, &pExplosion->position, &length0 );
			float2_sub( &capsule0.line.b, &pExplosion->position, &length1 );

			float2 length2;
			float2_set( &length2, 0.0f, pExplosion->length[ 1u ] );
			float2_rotate( &length2, pExplosion->direction );

			float2 length3;
			float2_set( &length3, 0.0f, pExplosion->length[ 3u ] );
			float2_rotate( &length3, pExplosion->direction );

			float2_add( &capsule1.line.a, &pExplosion->position, &length2 );
			float2_sub( &capsule1.line.b, &pExplosion->position, &length3 );

			for( uint j = 0u; j < SYS_COUNTOF( pServer->gameState.player ); ++j )
			{
				ServerPlayer* pPlayer = &pServer->gameState.player[ j ];
				if( pPlayer->playerState == PlayerState_InActive )
				{
					continue;
				}

				Circle playerCirlce;
				playerCirlce.center = pPlayer->position;
				playerCirlce.radius = s_carRadius;

				const int isPlayerOldEnough = pPlayer->age > s_playerBulletProofAge;

				if( isPlayerOldEnough && ( isCircleCapsuleIntersecting( &playerCirlce, &capsule0 ) || isCircleCapsuleIntersecting( &playerCirlce, &capsule1 ) ) )
				{
					if( pExplosion->player < MaxPlayer ) 
					{
						ServerPlayer* pFragPlayer = &pServer->gameState.player[ pExplosion->player ];
						if( pExplosion->player == j )
						{
							pFragPlayer->frags--;
						}
						else
						{
							pFragPlayer->frags++;
						}
					}

					player_respawn( pPlayer, &s_playerStartPositions[ j ], s_playerStartDirections[ j ] );
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
				bombCircle.radius = s_bombRadius;

				if( isCircleCapsuleIntersecting( &bombCircle, &capsule0 ) || isCircleCapsuleIntersecting( &bombCircle, &capsule1 ) )
				{
					bomb_explode( find_free_explosion( pServer->gameState.explosions, SYS_COUNTOF( pServer->gameState.explosions ) ), pBomb, pWorld );

					pBomb->time = 0.0f;
				}
			}

			for( uint j = 0u; j < SYS_COUNTOF( pServer->gameState.items ); ++j )
			{
				ServerItem* pItem = &pServer->gameState.items[ j ];
				if( pItem->type == ItemType_None )
				{
					continue;
				}

				Circle itemCircle;
				itemCircle.center = pItem->position;
				itemCircle.radius = s_itemRadius;

				if( isCircleCapsuleIntersecting( &itemCircle, &capsule0 ) || isCircleCapsuleIntersecting( &itemCircle, &capsule1 ) )
				{
					pItem->type = ItemType_None;
				}
			}
		}

		pExplosion->time += GAMETIMESTEP;
		if( pExplosion->time >= s_explosionTime )
		{
			pExplosion->time = 0.0f;
		}
	}

	for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.player ); ++i )
	{
		ServerPlayer* pPlayer = &pServer->gameState.player[ i ];
		if( pPlayer->playerState == PlayerState_InActive )
		{
			continue;
		}

		Circle playerCirlce;
		playerCirlce.center = pPlayer->position;
		playerCirlce.radius = s_carRadius;

		for( uint j = 0u; j < SYS_COUNTOF( pServer->gameState.bombs ); ++j )
		{
			const ServerBomb* pBomb = &pServer->gameState.bombs[ j ];
			if( pBomb->time > 0.0f ) 
			{
				Circle bombCircle;
				bombCircle.center = pBomb->position;
				bombCircle.radius = s_bombRadius;

				circleCircleCollide( &bombCircle, &playerCirlce, 1.0f, 0, &pPlayer->position );
			}
		}

		for( uint j = 0u; j < SYS_COUNTOF( pWorld->rockz ); ++j )
		{
			circleCircleCollide( &pWorld->rockz[ j ], &playerCirlce, 1.0f, 0, &pPlayer->position );
		}

		for( uint j = i + 1u; j < SYS_COUNTOF( pServer->gameState.player ); ++j )
		{
			ServerPlayer* pOtherPlayer = &pServer->gameState.player[ j ];
			if( pOtherPlayer->playerState == PlayerState_InActive )
			{
				continue;
			}

			Circle otherPlayerCirlce;
			otherPlayerCirlce.center = pOtherPlayer->position;
			otherPlayerCirlce.radius = s_carRadius;

			circleCircleCollide( &playerCirlce, &otherPlayerCirlce, 0.5f, &pPlayer->position, &pOtherPlayer->position );
		}
	}

	pServer->gameState.timeToNextItem -= GAMETIMESTEP;
	if( pServer->gameState.timeToNextItem <= 0.0f )
	{	
		for( uint i = 0u; i < SYS_COUNTOF( pServer->gameState.items ); ++i )
		{
			ServerItem* pItem = &pServer->gameState.items[ i ];

			if( pItem->type == ItemType_None )
			{
				if( server_findFreePosition( &pItem->position, pWorld ) )
				{
					pItem->type	= ( float_rand() < 0.5f ? ItemType_ExtraBomb : ItemType_BombRange );
				}				
				break;
			}
		}
		pServer->gameState.timeToNextItem = float_rand_range( s_itemMinTime, s_itemMaxTime );
	}

	pServer->gameState.id++;

	server_send_client_state( pServer );
}

