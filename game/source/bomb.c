#include "bomb.h"

void bomb_explode( Explosion* pExplosion, const Bomb* pBomb )
{
	pExplosion->position	= pBomb->position;
	pExplosion->direction	= pBomb->direction;
	pExplosion->length		= pBomb->length;
	pExplosion->time		= 0.0f;
}

uint bomb_update( Bomb* pBomb, Explosion* pExplosion )
{
	uint result = 0u;
	if( pBomb->active )
	{
		pBomb->time -= GAMETIMESTEP;
		if( pBomb->time <= 0.0f )
		{
			if( pExplosion != 0 )
			{
				bomb_explode( pExplosion, pBomb );
				result = 1u;
			}
			pBomb->active = 0;
		}
	}
	return result;
}

void bomb_place( Bomb* pBomb, const float2* pPosition, float direction, float length )
{
	pBomb->position		= *pPosition;
	pBomb->direction	= direction;
	pBomb->length		= length;
	pBomb->time			= 2.0f;
	pBomb->active		= 1;
}
