<Shared>
varying vec2 texCoord;

<VS>

void main()
{
    texCoord = gl_MultiTexCoord0;
    gl_Position = gl_Vertex;
};


<FS>
uniform sampler2D bgTexture;
uniform sampler2D fgTexture;

void main()
{
    vec4 bgColor = texture2D( bgTexture, texCoord );
    vec4 fgColor = texture2D( fgTexture, texCoord );

    vec4 result = bgColor * (1-fgColor.w) + fgColor;
    gl_FragColor = result;
};

