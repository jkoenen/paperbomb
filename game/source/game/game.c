#include "game/game.h"
#include "graphics/renderer.h"
#include "sys/debug.h"
#include "sys/types.h"
#include "game/player.h"

#include <string.h>

enum
{
	MaxPlayer = 8u
};

typedef struct
{
    float		gameTime;

	player_t	player[ MaxPlayer ];
	uint		playerCount;
} game_t;

static game_t s_game;

void game_init()
{
    SYS_TRACE_DEBUG( "game_init()\n" );
    renderer_init();

    s_game.gameTime = 0.0f;
	s_game.playerCount = 1u;
	for( uint i = 0u; i < s_game.playerCount; ++i )
	{
		player_reset( &s_game.player[ i ] );
	}
}

void game_done()
{
    renderer_done();
}

void game_update( const GameInput* pInput )
{
    const float timeStep = pInput->timeStep;

    s_game.gameTime += timeStep;

	player_update( &s_game.player[ 0u ], timeStep, pInput->buttonMask );
}

void game_render()
{
    FrameData frame;
    memset( &frame, 0u, sizeof( frame ) );
    frame.time = s_game.gameTime;
    frame.playerPos = s_game.player[ 0u ].position;
    renderer_drawFrame( &frame );
}

