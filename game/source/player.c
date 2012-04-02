#include "player.h"
#include "debug.h"
#include "input.h"
#include "vector.h"

void player_reset( Player* pPlayer )
{
	pPlayer->position.x		= 0.0f;
	pPlayer->position.y		= 0.0f;
	pPlayer->direction		= 0.0f;
	pPlayer->steer			= 0.0f;
	pPlayer->velocity.x		= 0.0f;
	pPlayer->position.y		= 0.0f;
	pPlayer->health			= 100.0f;
	pPlayer->lastButtonMask	= 0u;
	pPlayer->maxBombs		= 2u;
	for( uint i = 0u; i < SYS_COUNTOF( pPlayer->bombs ); ++i )
	{
		pPlayer->bombs[ i ].active = 0;
	}
}

#define M_PI_4F ((float)M_PI_4)

void player_update( Player* pPlayer, uint32 buttonMask )
{
	const uint32 buttonDownMask = buttonMask & ~pPlayer->lastButtonMask;

	const float steerSpeed = 0.02f;
	const float steerDamping = 0.9f;
	const float maxSpeed = 0.05f;
		
	if( buttonMask & ButtonMask_Left )
	{
		pPlayer->steer = float_min( pPlayer->steer + steerSpeed, M_PI_4F );
	}
	else if( buttonMask & ButtonMask_Right )
	{
		pPlayer->steer = float_max( pPlayer->steer - steerSpeed, -M_PI_4F );
	}
	else
	{
		pPlayer->steer *= steerDamping;
	}

	float acceleration = 0.0f;
	if( buttonMask & ButtonMask_Up )
	{
		acceleration = 0.005f;
	}
	else if( buttonMask & ButtonMask_Down )
	{
		acceleration = -0.005f;
	}

	if( buttonDownMask & ButtonMask_PlaceBomb )
	{
		int freeIndex = -1;
		uint activeBombCount = 0u;
		for( int i = 0u; i < SYS_COUNTOF( pPlayer->bombs ); ++i )
		{
			if( pPlayer->bombs[ i ].active )
			{
				activeBombCount++;
			}
			else
			{
				freeIndex = i;
			}
		}
		if( ( pPlayer->maxBombs > activeBombCount ) && ( freeIndex >= 0 ) )
		{
			Bomb* pBomb = &pPlayer->bombs[ freeIndex ];

			pBomb->position		= pPlayer->position;
			pBomb->direction	= pPlayer->direction;
			pBomb->length		= 8.0f;
			pBomb->time			= 5.0f;
			pBomb->active		= 1;
		}
	}

	for( uint i = 0u; i < SYS_COUNTOF( pPlayer->bombs ); ++i )
	{
		Bomb* pBomb = &pPlayer->bombs[ i ];
		if( pBomb->active )
		{
			pBomb->time -= 1.0f / 60.0f;
			if( pBomb->time <= 0.0f )
			{
				pBomb->active = 0;
			}
		}
	}

	float2 directionVector;
	float2_from_angle( &directionVector, pPlayer->direction );

	float2 velocityNormalized = pPlayer->velocity;
	float2_normalize0( &velocityNormalized );

	const float dirVecDot = float2_dot( &directionVector, &velocityNormalized );
	pPlayer->direction += dirVecDot * pPlayer->steer * 0.05f;

	float2 velocityForward = pPlayer->velocity;
	float2_scale1f( &velocityForward, float_abs( dirVecDot ) );

	float2 velocitySide;
	float2_sub( &velocitySide, &pPlayer->velocity, &velocityForward );

	float2 velocity;
	velocity.x = acceleration;
	velocity.y = 0.0f;

	float2_rotate( &velocity, pPlayer->direction );
	float2_addScaled1f( &velocity, &velocity, &velocityForward, 0.95f );
	float2_addScaled1f( &velocity, &velocity, &velocitySide, 0.7f );

	pPlayer->velocity = velocity; 

	const float speed = float_abs( float2_length( &pPlayer->velocity ) );
	if( speed > maxSpeed )
	{
		float2_scale1f( &pPlayer->velocity, maxSpeed / speed );
	}

	float2_add( &pPlayer->position, &pPlayer->position, &pPlayer->velocity );

	pPlayer->position.x = float_clamp( pPlayer->position.x, -16.0f, 16.0f );
	pPlayer->position.y = float_clamp( pPlayer->position.y, -9.0f, 9.0f );
	pPlayer->lastButtonMask = buttonMask;
}
