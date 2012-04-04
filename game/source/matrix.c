#include "matrix.h"


float2x2* float2x2_set( float2x2* pValue, float x0, float x1, float y0, float y1 )
{
    pValue->x.x = x0;
    pValue->x.y = x1;
    pValue->y.x = y0;
    pValue->y.y = y1;

    return pValue;
}

float2x2* float2x2_identity( float2x2* pValue )
{
    return float2x2_set( pValue, 1.0f, 0.0f, 0.0f, 1.0f );
}

float2x2* float2x2_rotationY( float2x2* pValue, float angleRad )
{
    const float sa = sinf( angleRad );
    const float ca = cosf( angleRad );

    return float2x2_set( pValue, ca, -sa, sa, ca );
}

float2x2* float2x2_scale1f( float2x2* pResult, const float2x2* pMatrix, float scale )
{
    const float m00 = pMatrix->x.x;
    const float m10 = pMatrix->x.y;
    const float m01 = pMatrix->y.x;
    const float m11 = pMatrix->y.y;

    const float r00 = m00 * scale;
    const float r10 = m10 * scale;
    const float r01 = m01 * scale;
    const float r11 = m11 * scale;

    return float2x2_set( pResult, r00, r10, r01, r11 );
}

float2x2* float2x2_scale2f( float2x2* pResult, const float2x2* pMatrix, float scaleX, float scaleY )
{
    const float m00 = pMatrix->x.x;
    const float m10 = pMatrix->x.y;
    const float m01 = pMatrix->y.x;
    const float m11 = pMatrix->y.y;

    const float r00 = m00 * scaleX;
    const float r10 = m10 * scaleX;
    const float r01 = m01 * scaleY;
    const float r11 = m11 * scaleY;

    return float2x2_set( pResult, r00, r10, r01, r11 );
}

float2* float2x3_transform( float2* pResult, const float2x3* pMatrix, const float2* pPoint )
{
    const float m00 = pMatrix->rot.x.x;
    const float m10 = pMatrix->rot.x.y;
    const float m01 = pMatrix->rot.y.x;
    const float m11 = pMatrix->rot.y.y;
    const float m02 = pMatrix->pos.x;
    const float m12 = pMatrix->pos.y;
    const float px = pPoint->x;
    const float py = pPoint->y;

    const float rx = m00 * px + m01 * py + m02;
    const float ry = m10 * px + m11 * py + m12;

    return float2_set( pResult, rx, ry );
}


