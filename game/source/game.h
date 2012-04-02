#ifndef GAME_MAIN_H_INCLUDED
#define GAME_MAIN_H_INCLUDED

#include "types.h"

typedef struct
{
    uint32      buttonMask;
    float       timeStep;
} GameInput;

void game_init();
void game_done();

void game_update( const GameInput* pInput );
void game_render();

#endif 

