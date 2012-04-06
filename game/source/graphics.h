#ifndef GRAPHICS_H_INCLUDED
#define GRAPHICS_H_INCLUDED

#include "types.h"
#include "rendertarget.h"
#include "shader.h"

typedef enum
{
    BlendMode_Disabled,
    BlendMode_Over,
    BlendMode_Count
} BlendMode;

typedef enum
{
    VertexFormat_None,
    VertexFormat_2d,
    VertexFormat_Count
} VertexFormat;

typedef enum
{
    SamplerState_ClampU_ClampV_Nearest,
    SamplerState_MirrorU_MirrorV_Bilinear,
    SamplerState_ClampU_ClampV_Trilinear,
    SamplerState_Count
} SamplerState;

typedef struct 
{
    float2  pos;
    float2  texCoord;
} Vertex2d;

typedef uint16 Index;

typedef struct
{
    uint    vertexCount;
    uint    indexCount;
    uint    vertexBufferId;
    uint    indexBufferId;    
} Mesh2d;

typedef struct
{
    Vertex2d*   pVertexData;
    Index*      pIndexData;
} Mesh2dLock;


void graphics_init();
void graphics_done();

void graphics_resetState();

void graphics_clear( float r, float g, float b, float a );
void graphics_drawFullscreenQuad();
void graphics_drawQuad( const float2* pVertices, float u0, float v0, float u1, float v1 );
void graphics_drawCircle( const float2* pPos, float radius );

void graphics_setRenderTarget( RenderTarget* pTarget );
void graphics_setShader( Shader* pShader );

void graphics_setVp4f( uint index, float x, float y, float z, float w );
void graphics_setFp4f( uint index, float x, float y, float z, float w );
void graphics_setFsTexture( uint index, uint textureId, SamplerState sampler );

void graphics_setVertexFormat( VertexFormat format );
void graphics_setBlendMode( BlendMode mode );

void graphics_createMesh2d( Mesh2d* pMesh, uint vertexCount, uint indexCound );
int graphics_lockMesh2d( Mesh2dLock* pLock, Mesh2d* pMesh );
void graphics_unlockMesh2d( Mesh2d* pMesh );
void graphics_drawMesh2d( const Mesh2d* pMesh );

#endif

