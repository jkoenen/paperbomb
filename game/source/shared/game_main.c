#include <stdlib.h>

typedef struct 
{
    float   gameTime;
} pb_game;

pb_game* pb_game_init()
{
    pb_game* pGame = ( pb_game* )malloc( sizeof( pb_game ) );

    pGame->gameTime = 0.0f;

    return pGame;
}

void pb_game_done( pb_game* pGame )
{
    free( pGame );
}

void pb_game_update( pb_game* pGame, float timeStep )
{
    pGame->gameTime += timeStep;
}

void pb_game_render( const pb_game* pGame )
{
    (void)pGame;
}

