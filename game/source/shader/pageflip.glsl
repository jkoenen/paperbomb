<Shared>
varying vec2 texCoord;

<VS>
uniform vec4 vp0;
void main()
{
    float progress=vp0.x;
    texCoord = gl_MultiTexCoord0.xy;
    vec4 pos = gl_Vertex;
    pos.y += progress * sin( 6.2f * texCoord.x );//progress;
    gl_Position = pos;
}

<FS>
uniform sampler2D ft0;
uniform sampler2D ft1;

void main()
{
    vec4 bgColor = texture2D( ft0, texCoord );
    vec4 fgColor = texture2D( ft1, texCoord );

    vec4 result = bgColor * (1.0-fgColor.w) + fgColor;
//result=fgColor;
    gl_FragColor = result;
}

