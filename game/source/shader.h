#ifndef GRAPHICS_SHADER_H_INCLUDED
#define GRAPHICS_SHADER_H_INCLUDED

#include "glslshader.h"

enum
{
    MaxVSUniformCount = 4,
    MaxFSUniformCount = 4,
    MaxFSTextureCount = 4
};

typedef struct
{
    uint    id;
    int     vp[ MaxVSUniformCount ];
    int     fp[ MaxFSUniformCount ];
    int     ft[ MaxFSTextureCount ];
} Shader;

uint shader_create( Shader* pShader, const GlslShaderDefinition* pDefinition, uint vsUniformCount, uint fsUniformCount, uint fsTextureCount );
void shader_activate( const Shader* pShader );

void shader_setVp4f( const Shader* pShader, uint index, float x, float y, float z, float w );
void shader_setFp4f( const Shader* pShader, uint index, float x, float y, float z, float w );

#endif

