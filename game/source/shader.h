#ifndef GRAPHICS_SHADER_H_INCLUDED
#define GRAPHICS_SHADER_H_INCLUDED

#include "glslshader.h"

enum
{
    MaxVSUniformCount = 8,
    MaxFSUniformCount = 8
};

typedef struct
{
    uint     id;
    int     vsUniforms[ MaxVSUniformCount ];
    int     fsUniforms[ MaxFSUniformCount ];
} Shader;

uint shader_create( Shader* pShader, const GlslShaderDefinition* pDefinition );
void shader_activate( const Shader* pShader );

#endif

