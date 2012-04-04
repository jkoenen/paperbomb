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

uniform vec4 fp0;
uniform vec4 fp1;
void main()
{
    float width=0.25f; ///18.0;    // * rand( gl_FragCoord );
    float offset=0.0f; //fp0.y;
    float curveSize=fp0.z;
    vec4 penColor=vec4( fp1.xyz, 1.0f );

    float pixelPos=texCoord.y;
    float linepos=0.5f;//-curveSize*sin(texCoord.x*3.14159+offset);
 
    float distance = abs( linepos - pixelPos );
    float x=(distance/width);
    float strokeVariance=mix(0.7f,0.8f,cos(texCoord.x*3.14159));

    float intensity =strokeVariance*max(1.0-x*x,0.0);

    vec4 color=penColor*intensity;
    gl_FragColor=color;
}

