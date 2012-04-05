#include "font.h"
#include "vector.h"
#include "renderer.h"

#include <string.h>
#include <memory.h>

enum
{
    Font_FirstGlyph = 33,
    Font_LastGlyph = 128
};

typedef struct
{
    uint                characterCode;
    float               advance;
    const float2*       pPoints;
    uint                pointCount;
} FontGlyph;

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
    { 0.0f, 2.0f },
    { 0.0f, 0.0f },
    { 2.0f, 0.0f },
    { 4.0f, 0.0f },
    { 4.0f, 2.0f },
    { 4.0f, 4.0f },
    { 2.0f, 0.0f },
    { 0.0f, 4.0f },
    { 0.0f, 2.0f }
};

static const float2 s_points_1[] =
{
    { 0.0f, 1.9f },
    { 0.0f, 1.9f },
    { 1.1f, 3.0f },
    { 1.1f, 2.0f },
    { 1.1f, 0.0f }
};

static const FontGlyph s_glyphs[] =
{
    { 0, 0.0f, 0, 0 },
    /*{ 'A', 9.0f, s_points_a, SYS_COUNTOF( s_points_a ) },
    { 'H', 8.0f, s_points_h, SYS_COUNTOF( s_points_h ) },
    { 'L', 7.0f, s_points_l, SYS_COUNTOF( s_points_l ) },
    { 'O', 9.0f, s_points_o, SYS_COUNTOF( s_points_o ) },*/
    { 'O', 5.0f, s_points_0, SYS_COUNTOF( s_points_0 ) },
    { '1', 2.0f, s_points_1, SYS_COUNTOF( s_points_1 ) },
/*    { '2', 9.0f, s_points_2, SYS_COUNTOF( s_points_2 ) },
    { '3', 9.0f, s_points_3, SYS_COUNTOF( s_points_3 ) },
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

void font_drawText( const float2* pPosition, float size, float variance, const char* pText )
{
    float2x3 transform;
    float2x2_rotationY( &transform.rot, float_rand_normal( 0.0f, DEG2RADF( variance * 10.0f ) ) );
    float2x2_scale1f( &transform.rot, &transform.rot, size );
    transform.pos = *pPosition;

    while( *pText )
    {
        const uint charCode = (uint)*pText++;
        const uint glyphIndex = s_font.glyphMap[ charCode ];
        if( !glyphIndex )
        {
            continue;
        }

        const FontGlyph* pGlyph = &s_glyphs[ glyphIndex ];
        if( pGlyph->pointCount < 3 )
        {
            continue;
        }
        
        renderer_setTransform( &transform );
        renderer_addLinearStroke( pGlyph->pPoints, pGlyph->pointCount );

        const float advance = float_rand_normal( pGlyph->advance, variance );
        float2_addScaled1f( &transform.pos, &transform.pos, &transform.rot.x, advance );
    }
}

