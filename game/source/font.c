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

static const float2 s_points_0[] =
{ 
    { 1.0f, 3.0f },
    { 0.1f, 3.0f },
    { 0.1f, 1.5f },
    { 0.1f, 0.0f },
    { 1.0f, 0.0f },
    { 2.0f, 0.0f },
    { 2.0f, 1.5f },
    { 2.0f, 3.0f },
    { 1.0f, 3.0f }
};

static const float2 s_points_1[] =
{
    { 0.0f, 1.9f },
    { 0.0f, 1.9f },
    { 1.1f, 3.0f },
    { 1.1f, 3.0f },
    { 1.1f, 0.0f }
};

static const float2 s_points_2[] =
{
    { 0.2f, 2.7f },
    { 1.0f, 3.5f },
    { 1.9f, 2.2f },
    { 1.2f, 0.8f },
    { 0.0f, 0.0f },
    { 0.0f, 0.0f },
    { 1.9f, 0.0f }
};

static const float2 s_points_3[] =
{
    { 0.0f, 2.1f },
    { 0.0f, 3.0f },
    { 1.0f, 3.0f },
    { 1.9f, 3.0f },
    { 1.9f, 1.9f },
    { 1.9f, 0.0f },
    { 0.0f, 0.0f },
    { 0.0f, 0.0f },
    { 1.9f, 0.0f }
};

static const float2 s_points_4[] =
{
    { 0.0f, 2.1f },
    { 0.0f, 3.0f },
    { 1.0f, 3.0f },
    { 1.9f, 3.0f },
    { 1.9f, 1.9f },
    { 1.9f, 0.0f },
    { 0.0f, 0.0f },
    { 0.0f, 0.0f },
    { 1.9f, 0.0f }
};

static const float2 s_points_5[] =
{
    { 0.0f, 2.1f },
    { 0.0f, 3.0f },
    { 1.0f, 3.0f },
    { 1.9f, 3.0f },
    { 1.9f, 1.9f },
    { 1.9f, 0.0f },
    { 0.0f, 0.0f },
    { 0.0f, 0.0f },
    { 1.9f, 0.0f }
};

static const float2 s_points_6[] =
{
    { 0.0f, 2.1f },
    { 0.0f, 3.0f },
    { 1.0f, 3.0f },
    { 1.9f, 3.0f },
    { 1.9f, 1.9f },
    { 1.9f, 0.0f },
    { 0.0f, 0.0f },
    { 0.0f, 0.0f },
    { 1.9f, 0.0f }
};

static const float2 s_points_7[] =
{
    { 0.0f, 2.1f },
    { 0.0f, 3.0f },
    { 1.0f, 3.0f },
    { 1.9f, 3.0f },
    { 1.9f, 1.9f },
    { 1.9f, 0.0f },
    { 0.0f, 0.0f },
    { 0.0f, 0.0f },
    { 1.9f, 0.0f }
};

static const float2 s_points_8[] =
{
    { 0.0f, 2.1f },
    { 0.0f, 3.0f },
    { 1.0f, 3.0f },
    { 1.9f, 3.0f },
    { 1.9f, 1.9f },
    { 1.9f, 0.0f },
    { 0.0f, 0.0f },
    { 0.0f, 0.0f },
    { 1.9f, 0.0f }
};

static const float2 s_points_9[] =
{
    { 0.0f, 2.1f },
    { 0.0f, 3.0f },
    { 1.0f, 3.0f },
    { 1.9f, 3.0f },
    { 1.9f, 1.9f },
    { 1.9f, 0.0f },
    { 0.0f, 0.0f },
    { 0.0f, 0.0f },
    { 1.9f, 0.0f }
};

static const FontGlyph s_glyphs[] =
{
    { 0, 0.0f, 0, 0 },
    /*{ 'A', 9.0f, s_points_a, SYS_COUNTOF( s_points_a ) },
    { 'H', 8.0f, s_points_h, SYS_COUNTOF( s_points_h ) },
    { 'L', 7.0f, s_points_l, SYS_COUNTOF( s_points_l ) },
    { 'O', 9.0f, s_points_o, SYS_COUNTOF( s_points_o ) },*/
    { '0', 5.0f, s_points_0, SYS_COUNTOF( s_points_0 ) },
    { '1', 2.0f, s_points_1, SYS_COUNTOF( s_points_1 ) },
    { '2', 9.0f, s_points_2, SYS_COUNTOF( s_points_2 ) },
    /*{ '3', 9.0f, s_points_3, SYS_COUNTOF( s_points_3 ) },
    { '4', 9.0f, s_points_4, SYS_COUNTOF( s_points_4 ) },
    { '5', 9.0f, s_points_5, SYS_COUNTOF( s_points_5 ) },
    { '6', 9.0f, s_points_6, SYS_COUNTOF( s_points_6 ) },
    { '7', 9.0f, s_points_7, SYS_COUNTOF( s_points_7 ) },
    { '8', 9.0f, s_points_8, SYS_COUNTOF( s_points_8 ) },
    { '9', 9.0f, s_points_9, SYS_COUNTOF( s_points_9 ) },*/
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
        if(pGlyph->pointCount<3u)
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

