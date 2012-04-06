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

static const float2 s_points_a[] =
{
    { 0.0f, 0.0f },
    { 4.0f, 10.0f },
    { 8.0f, 0.0f }, { 8.0f, 0.0f },     // end of stroke marker
    { 2.0f, 5.0f },
    { 6.0f, 5.0f }
};

static const float2 s_points_h[] =
{
    { 0.0f, 10.0f },
    { 0.0f, 0.0f }, { 0.0f, 0.0f },
    { 7.0f, 10.0f },
    { 7.0f, 0.0f }, { 7.0f, 0.0f },
    { 0.0f, 5.0f },
    { 7.0f, 5.0f }
};

static const float2 s_points_l[] =
{
    { 0.0f, 10.0f },
    { 0.0f, 0.0f },
    { 6.0f, 0.0f },
};

static const float2 s_points_o[] =
{
    // :TODO: .. linear won't cut it here...
    { 4.0f, 10.0f },
    { 0.0f, 5.0f },
    { 4.0f, 0.0f },
    { 8.0f, 5.0f },
    { 4.0f, 10.0f }
};

#include "font/char_48.h"
#include "font/char_49.h"
#include "font/char_50.h"
#include "font/char_51.h"
#include "font/char_52.h"
#include "font/char_53.h"
#include "font/char_54.h"
#include "font/char_55.h"
#include "font/char_56.h"
#include "font/char_57.h"

static const FontGlyph s_glyphs[] =
{
    { 0, 0.0f, 0, 0 },
    /*{ 'A', 9.0f, s_points_a, SYS_COUNTOF( s_points_a ) },
    { 'H', 8.0f, s_points_h, SYS_COUNTOF( s_points_h ) },
    { 'L', 7.0f, s_points_l, SYS_COUNTOF( s_points_l ) },
    { 'O', 9.0f, s_points_o, SYS_COUNTOF( s_points_o ) },*/
    { '0', 3.0f, s_points_48, SYS_COUNTOF( s_points_48 ) },
    { '1', 2.0f, s_points_49, SYS_COUNTOF( s_points_49 ) },
    { '2', 3.0f, s_points_50, SYS_COUNTOF( s_points_50 ) },
    { '3', 3.0f, s_points_51, SYS_COUNTOF( s_points_51 ) },
    { '4', 3.0f, s_points_52, SYS_COUNTOF( s_points_52 ) },
    { '5', 3.0f, s_points_53, SYS_COUNTOF( s_points_53 ) },
    { '6', 3.0f, s_points_54, SYS_COUNTOF( s_points_54 ) },
    { '7', 2.5f, s_points_55, SYS_COUNTOF( s_points_55 ) },
    { '8', 3.0f, s_points_56, SYS_COUNTOF( s_points_56 ) },
    { '9', 3.0f, s_points_57, SYS_COUNTOF( s_points_57 ) },
};

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

        s_font.glyphMap[ pGlyph->characterCode ] = i;
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

    return &s_glyphs[ glyphIndex ];
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
        if(!pGlyph||pGlyph->pointCount<3u)
        {
            continue;
        }
        
        renderer_setTransform( &transform );
        renderer_addQuadraticStroke( pGlyph->pPoints, pGlyph->pointCount );

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

