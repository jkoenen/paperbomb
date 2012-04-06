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
uniform sampler2D ft2;

void main()
{
    vec4 bgColor = texture2D( ft0, texCoord );
    vec4 fgColor = texture2D( ft1, texCoord );
    vec4 burnAmount = texture2D( ft2, texCoord );

    float burn=burnAmount.x;
    vec4 burnColor=mix(vec4(103.0/255.0,52.0/255.0,0.0/255.0,1.0f),vec4(0,0,0,1.0f),10.0*burn-5.0)*burn;

    fgColor=fgColor*(1.0-burnColor.w)+burnColor;
    
    vec4 result=bgColor*(1.0-fgColor.w)+fgColor;
    gl_FragColor=result;

//gl_FragColor=vec4(burn,burn,burn,1.0);
}

