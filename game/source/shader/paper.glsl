<Shared>
varying vec2 paperPos;
varying vec2 texCoord;

<VS>
uniform vec4 vp0;
void main()
{
    vec2 paperSize = vp0.xy;
    vec2 paperOffset = vp0.zw;
    paperPos = gl_Vertex.xy * paperSize + paperSize;
    texCoord = gl_Vertex.xy*vec2(0.5,0.5)+vec2(0.5,0.5);
    gl_Position = gl_Vertex;
}

<FS>

uniform vec4 fp0;
uniform sampler2D ft0;

void main()
{    
    vec2 noiseOffset=fp0.xy;
    vec2 gridOffset=fp0.xy;
    vec2 pixelSize=fp0.zw;

    vec4 noise=vec4(2.0,2.0,2.0,2.0)*texture2D(ft0,texCoord+noiseOffset)-vec4(1.0,1.0,1.0,1.0);
    float n=(noise.x+0.5*noise.y+0.25*noise.z+0.125*noise.w);

    vec2 s=vec2( 
        smoothstep( 0.0f, 72.0f * pixelSize.x, fract( paperPos.x ) ),
        smoothstep( 0.0f, 36.0f * pixelSize.y, fract( paperPos.y ) ) );
    
    float attn=0.4*(1.0-sqrt(texCoord.x*texCoord.x+texCoord.y+texCoord.y)/2.0)+0.6;

    vec3 light0=vec3(1.0,1.0,1.0);

    //vec2 s=smoothstep(vec2(0.01,0.01),vec2(0.1,0.1),fract(paperPos+gridOffset/10.0f));
    vec3 gridColor = vec3( 108.0/255.0, 101.0/255.0, 91.0/255.0 );
    vec3 paperColor0 = vec3( 190.0/255.0, 187.0/255.0, 168.0/255.0 )*1.3;
    vec3 paperColor1 = vec3( 197.0/255.0, 190.0/255.0, 172.0/255.0 )*1.3;
    vec3 paperColor = attn*mix(paperColor0, paperColor1, n );
    gl_FragColor = vec4( mix( gridColor, paperColor, s.x*s.y), 1.0);
//gl_FragColor=vec4(attn,attn,attn,1.0);
}

