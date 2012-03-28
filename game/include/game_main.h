#ifndef PB_GAME_MAIN_H_INCLUDED
#define PB_GAME_MAIN_H_INCLUDED

struct pb_game;
typedef struct pb_game pb_game_t;

pb_game_t* pb_game_init();
void pb_game_done( pb_game_t* pGame );
void pb_game_update( pb_game_t* pGame, float timeStep );
void pb_game_render( const pb_game_t* pGame );

#endif 

