#include "player.h"
#include "debug.h"
#include "input.h"
#include "vector.h"

void player_init( Player* pPlayer, const float2* pPosition, float direction, int clearBombs )
{
	pPlayer->age			= 0.0f;
	pPlayer->position		= *pPosition;
	pPlayer->direction		= direction;
	pPlayer->steer			= 0.0f;
	pPlayer->velocity.x		= 0.0f;
	pPlayer->position.y		= 0.0f;
	pPlayer->health			= 100.0f;
	pPlayer->maxBombs		= 4u;
	pPlayer->bombLength		= 8.0f;

	if( clearBombs )
	{
		for( uint i	= 0u; i < SYS_COUNTOF( pPlayer->bombs ); ++i )
		{
			pPlayer->bombs[ i ].active = 0;
		}
	}
}

void player_update_input( Player* pPlayer, uint32 buttonMask, uint32 buttonDownMask )
{
	const float steerSpeed = 0.05f;
	const float steerDamping = 0.8f;
	const float maxSpeed = 0.5f;
	const float maxSteer = (float)PI * 0.2f;

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
		int freeIndex = -1;
		uint activeBombCount = 0u;
		for( uint i = 0u; i < SYS_COUNTOF( pPlayer->bombs ); ++i )
		{
			if( pPlayer->bombs[ i ].active )
			{
				activeBombCount++;
			}
			else if( freeIndex < 0 )
			{
				freeIndex = (int)i;
			}
		}
		if( ( pPlayer->maxBombs > activeBombCount ) && ( freeIndex >= 0 ) )
		{
			Bomb* pBomb = &pPlayer->bombs[ freeIndex ];

			bomb_place( pBomb, &pPlayer->position, pPlayer->direction, pPlayer->bombLength );
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
	float2_scale1f( &velocityForward, float_abs( dirVecDot ) );

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
		float2_scale1f( &pPlayer->velocity, maxSpeed / speed );
	}

//	SYS_TRACE_DEBUG( "speed %.4f\n", speed );

	float2_add( &pPlayer->position, &pPlayer->position, &pPlayer->velocity );

	pPlayer->position.x = float_clamp( pPlayer->position.x, -32.0f, 32.0f );
	pPlayer->position.y = float_clamp( pPlayer->position.y, -18.0f, 18.0f );
}
