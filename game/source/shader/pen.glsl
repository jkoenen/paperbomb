<Shared>
varying vec2 texCoord;

<VS>

void main()
{
    texCoord = gl_MultiTexCoord0.xy;

    vec2 paperSize = vec2( 64.0, 36.0 );

    vec2 paperPos = gl_Vertex.xy;
    vec2 clipPos = vec2(2.0f*paperPos.x/paperSize.x,2.0f*(paperSize.y-paperPos.y)/paperSize.y)-vec2(1.0f,1.0f);

    gl_Position = vec4(clipPos,0.0f,1.0f);
}

<FS>

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

uniform vec4 params0;
void main()
{
    float width = 0.25f; ///18.0;    // * rand( gl_FragCoord );
    float offset = params0.y;
    float curveSize = params0.z;

    float pixelPos = texCoord.y;
    float linepos = 0.5f + curveSize*sin(texCoord.x*3.14159+offset);
 
    float distance = abs( linepos - pixelPos );
    float x=(distance/width);

    float intensity = mix(0.7f,0.8f,cos(texCoord.x*3.14159))*max(1.0-x*x,0.0);

    vec4 color=vec4(0.1,0.1,0.1,1.0)*intensity;
//color.xw=vec2(1.0,1.0);//=vec4(1,0,1,1);
//color=vec4(1,0,1,1);
    gl_FragColor=color;
}

