#include "bomb.h"

void bomb_update( Bomb* pBomb, Explosion* pExplosions, uint capacity )
{
	if( pBomb->active )
	{
		pBomb->time -= GAMETIMESTEP;
		if( pBomb->time <= 0.0f )
		{
			for( uint i = 0u; i < capacity; ++i, ++pExplosions )
			{
				if( !pExplosions->active )
				{
					pExplosions->active		= 1;
					pExplosions->positon	= pBomb->position;
					pExplosions->direction	= pBomb->direction;
					pExplosions->length		= pBomb->length;
					pExplosions->time		= 0.0f;
					break;
				}
			}
			pBomb->active = 0;
		}
	}
}