#ifndef MATRIX_H_INCLUDED
#define MATRIX_H_INCLUDED

#include "vector.h"

typedef struct
{
    float2      x;
    float2      y;
} float2x2;

typedef struct 
{
    float2x2    rot;
    float2      pos;
} float2x3;

float2x2* float2x2_set( float2x2* pValue, float x0, float x1, float y0, float y1 );
float2x2* float2x2_identity( float2x2* pValue );
float2x2* float2x2_rotationY( float2x2* pValue, float angleRad );
float2x2* float2x2_scale1f( float2x2* pResult, const float2x2* pMatrix, float scale );
float2x2* float2x2_scale2f( float2x2* pResult, const float2x2* pMatrix, float scaleX, float scaleY );

float2* float2x3_transform( float2* pResult, const float2x3* pMatrix, const float2* pPoint );
float2* float2x2_transform( float2* pResult, const float2x2* pMatrix, const float2* pPoint );

#endif

