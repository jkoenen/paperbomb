#include <stdlib.h>

typedef struct pb_game
{
    float   gameTime;
} pb_game_t;

pb_game_t* pb_game_init()
{
    pb_game_t* pGame = ( pb_game_t* )malloc( sizeof( pb_game_t ) );

    pGame->gameTime = 0.0f;

    return pGame;
}

void pb_game_done( pb_game_t* pGame )
{
    free( pGame );
}

void pb_game_update( pb_game_t* pGame, float timeStep )
{
    pGame->gameTime += timeStep;
}

void pb_game_render( const pb_game_t* pGame )
{
    (void)pGame;
}

