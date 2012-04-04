#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#incude "types.h"

typedef struct 
{
    float2  pos;
    float2  texCoord;
    float   pressure;
} Vertex2d;

typedef uint16 Index;

typedef struct
{
    uint    vertexCount;
    uint    indexCount;
    int     vbId;
    int     ibId;    
} Mesh2d;

typedef struct
{
    Vertex2d*   pVertexData;
    Index*      pIndexData;
} Mesh2dLock;

void mesh_create2d( Mesh2d* pMesh, uint vertexCount, uint indexCound );
int mesh_lock2d( Mesh2dLock* pLock, Mesh2d* pMesh );
void mesh_unlock2d( Mesh2d* pMesh );
void mesh_drawMesh2d( const Mesh2d* pMesh );

#endif

