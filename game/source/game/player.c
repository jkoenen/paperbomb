#include "game/player.h"
#include "sys/debug.h"
#include "game/input.h"

#include <math.h>

void player_reset( player_t* pPlayer )
{
	pPlayer->position.x = 0.0f;
	pPlayer->position.y = 0.0f;
	pPlayer->direction = 0.0f;
	pPlayer->steer = 0.0f;
	pPlayer->velocity.x = 0.0f;
	pPlayer->position.y = 0.0f;
	pPlayer->health = 100u;
	pPlayer->lastInputMask = 0u;
};

#define M_PI_4 (3.14159265f/4.0f)
void player_update( player_t* pPlayer, float timeStep, uint32 inputMask )
{
	const uint32 buttonDownMask = inputMask & ~pPlayer->lastInputMask;

	float accelerate = 0.0f;
	if( inputMask & ButtonMask_Left )
	{
		pPlayer->steer = float_clamp( pPlayer->steer + 0.1f * timeStep, -M_PI_4, M_PI_4 );
	}
	else if( inputMask & ButtonMask_Right )
	{
		pPlayer->steer = float_clamp( pPlayer->steer - 0.1f * timeStep, -M_PI_4, M_PI_4 );
	}
	else
	{
		pPlayer->steer *= 0.9f;
	}

	if( inputMask & ButtonMask_Up )
	{
		accelerate = 1.0f;
	}
	else if( inputMask & ButtonMask_Down )
	{
		accelerate -= 1.0f;
	}

	if( buttonDownMask & ButtonMask_PlaceBomb )
	{
		SYS_TRACE_DEBUG( "place bomb!\n" );
	}

	const float speed = timeStep * accelerate;

	pPlayer->direction += pPlayer->steer;

	float2 velocity;
	velocity.x = speed;
	velocity.y = 0.0f;

	velocity.x = velocity.x * cos( pPlayer->direction ) - velocity.y * sin( pPlayer->direction );
	velocity.y = velocity.y * cos( pPlayer->direction ) + velocity.x * sin( pPlayer->direction );

	float2_scale( &pPlayer->velocity, 0.9f );
	float2_add( &pPlayer->velocity, &pPlayer->velocity, &velocity );

	float2_add( &pPlayer->position, &pPlayer->position, &pPlayer->velocity );

	pPlayer->position.x = float_clamp( pPlayer->position.x, -1.0f, 1.0f );
	pPlayer->position.y = float_clamp( pPlayer->position.y, -1.0f, 1.0f );
	pPlayer->lastInputMask = inputMask;

}
