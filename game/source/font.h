#ifndef FONT_H_INCLUDED
#define FONT_H_INCLUDED

#include "renderer.h"


void font_init();
void font_done();

void font_drawText( const float2* pPosition, float size, float width, float variance, const char* pText );

#endif

