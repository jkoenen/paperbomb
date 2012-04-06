#ifndef FONT_H_INCLUDED
#define FONT_H_INCLUDED

#include "renderer.h"

typedef struct
{
    uint                characterCode;
    float               advance;
    const float2*       pPoints;
    const uint8*        pCommands;
    uint                commandCount;
} FontGlyph;

void font_init();
void font_done();

const FontGlyph* font_getGlyph( uint charCode );
void font_drawText( const float2* pPosition, float size, float variance, const char* pText );

uint font_getNextGlyphCharCode( uint charCode, int direction );

#endif

