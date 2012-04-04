<Shared>
varying vec2 texCoord;

<VS>

void main()
{
    texCoord = gl_MultiTexCoord0.xy;
    gl_Position = gl_Vertex;
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

