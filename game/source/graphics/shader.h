#ifndef GRAPHICS_SHADER_H_INCLUDED
#define GRAPHICS_SHADER_H_INCLUDED

#include "graphics/glslshader.h"

enum
{
    MaxVSUniformCount = 8,
    MaxFSUniformCount = 8
};

typedef struct
{
    int     id;
    int     vsUniforms[ MaxVSUniformCount ];
    int     fsUniforms[ MaxFSUniformCount ];
} Shader;

int shader_create( Shader* pShader, const GlslShaderDefinition* pDefinition );
void shader_activate( const Shader* pShader );

#endif

