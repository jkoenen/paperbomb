#include "font.h"
#include "vector.h"

#include <string.h>

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

static const FontGlyph s_glyphs[] =
{
    { 0, 0.0f, 0, 0 },
    { 'A', 9.0f, s_points_a, SYS_COUNTOF( s_points_a ) },
    { 'H', 8.0f, s_points_h, SYS_COUNTOF( s_points_h ) },
    { 'L', 7.0f, s_points_l, SYS_COUNTOF( s_points_l ) },
    { 'O', 9.0f, s_points_o, SYS_COUNTOF( s_points_o ) },
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

    //float2x2_identity( &tansform.rot );
    float2x2_rotationY( &transform.rot, float_rand_normal( 0.0f, DEG2RADF( variance * 10.0f ) ) );
    float2x2_scale1f( &transform.rot, &transform.rot, size );
    transform.pos = *pPosition;

    //float2_add( &transform.pos, pPosition, &positionOffset );

    float2 basePos;
    float2_rand_normal( &basePos, 0.0f, variance );

    while( *pText )
    {
        const uint charCode = (uint)*pText++;
        const uint glyphIndex = s_font.glyphMap[ charCode ];
        if( !glyphIndex )
        {
            continue;
        }

        const FontGlyph* pGlyph = &s_glyphs[ glyphIndex ];
        if( pGlyph->pointCount <= 0 )
        {
            continue;
        }

        // search for     
        StrokeDefinition stroke;
        stroke.pPoints = pGlyph->pPoints;
        stroke.pointCount = 1u;

        float2 lastPoint = stroke.pPoints[ 0u ];
        for( uint i = 1u; i < pGlyph->pointCount; ++i )
        {
            const float2 currentPoint = pGlyph->pPoints[ i ];

            if( currentPoint.x == lastPoint.x && currentPoint.y == lastPoint.y )
            {
                if( stroke.pointCount >= 2u )
                {
                    float2x3 drawTransform;
                    drawTransform.rot = transform.rot;
                    float2x3_transform( &drawTransform.pos, &transform, &basePos );
                    renderer_addStroke( &stroke, Pen_Font, &drawTransform, variance );
                }
                stroke.pointCount = 0u;
                stroke.pPoints = &pGlyph->pPoints[ i + 1u ];
            }
            else
            {
                stroke.pointCount++;
            }
            lastPoint = currentPoint;
        }

        if( stroke.pointCount >= 2u )
        {
            float2x3 drawTransform;
            drawTransform.rot = transform.rot;
            float2x3_transform( &drawTransform.pos, &transform, &basePos );
            renderer_addStroke( &stroke, Pen_Font, &drawTransform, variance );
        }
    
        basePos.x += pGlyph->advance + float_rand_normal( 0.0f, variance );
        basePos.y = float_rand_normal( 0.0f, variance );
    }
}

