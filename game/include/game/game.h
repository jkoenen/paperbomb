#ifndef GAME_MAIN_H_INCLUDED
#define GAME_MAIN_H_INCLUDED

#include "sys/types.h"

enum
{
    ButtonMask_Left = 1u << 0u,
    ButtonMask_Right = 1u << 1u,
    ButtonMask_Up = 1u << 2u,
    ButtonMask_Down = 1u << 3u,
    ButtonMask_PlaceBomb = 1u << 4u
};

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

