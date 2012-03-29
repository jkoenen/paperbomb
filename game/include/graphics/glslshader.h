#ifndef GRAPHICS_GLSLSHADER_H_INCLUDED
#define GRAPHICS_GLSLSHADER_H_INCLUDED

typedef struct
{
    const char*     pVsCode;
    const char*     pFsCode;
    const char**    pVsUniformNames;
    int             vsUniformCount;
    const char**    pFsUniformNames;
    int             fsUniformCount;
} GlslShaderDefinition;

#endif

