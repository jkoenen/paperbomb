#include "font.h"
#include "vector.h"
#include "renderer.h"
#include "debug.h"

#include <string.h>
#include <memory.h>

enum
{
    Font_FirstGlyph = 33,
    Font_LastGlyph = 128
};

#include "font2.h"

typedef struct
{
    uint  glyphMap[ 256u ];
} Font;

static Font s_font;

void font_init()
{
    memset( s_font.glyphMap, 0, sizeof( s_font.glyphMap ) );

    for( uint i = 0u; i < SYS_COUNTOF( s_glyphs ); ++i )
    {
        const FontGlyph* pGlyph = &s_glyphs[ i ];
        
    //    SYS_TRACE_DEBUG("adding glyph for char code %i @ %i\n", pGlyph->characterCode,i);
        s_font.glyphMap[ pGlyph->characterCode ] = i+1u;
    }
}

void font_done()
{
}

const FontGlyph* font_getGlyph( uint charCode )
{
    if(charCode >= SYS_COUNTOF(s_font.glyphMap))
    {
        return 0;
    }

    const uint glyphIndex = s_font.glyphMap[ charCode ];
    if( !glyphIndex )
    {
        return 0;
    }

    return &s_glyphs[ glyphIndex - 1u ];
}

void font_drawText( const float2* pPosition, float size, float variance, const char* pText )
{
    float2x3 transform;
    float2x2_rotationY( &transform.rot, float_rand_normal( 0.0f, DEG2RADF( variance * 10.0f ) ) );
    float2x2_scale1f( &transform.rot, &transform.rot, size );
    transform.pos = *pPosition;

    while( *pText )
    {
        const uint charCode = (uint)*pText++;
        const FontGlyph* pGlyph = font_getGlyph( charCode );
        if(!pGlyph)
        {
            //SYS_TRACE_WARNING("Could not find glyph for char code %i\n", charCode);
            continue;
        }
        renderer_setTransform( &transform );
        renderer_addCommandStroke( pGlyph->pPoints, pGlyph->pCommands, pGlyph->commandCount );

        const float advance = float_rand_normal( pGlyph->advance, variance );
        float2_addScaled1f( &transform.pos, &transform.pos, &transform.rot.x, advance );
    }
}

uint font_getNextGlyphCharCode( uint charCode, int direction )
{
    const uint count=SYS_COUNTOF(s_font.glyphMap);
    for(uint i=1u;i<count;++i)
    {
        uint index;
        if(direction>0)
        {
            index=charCode+i;
        }
        else
        {
            index=charCode+count-i;
        }
        index=index%count;

        if( s_font.glyphMap[index] )
        {
            return index;
        }
    }
    return charCode;
}

