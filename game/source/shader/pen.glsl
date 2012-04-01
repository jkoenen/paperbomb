<Shared>
varying vec2 texCoord;

<VS>

void main()
{
    texCoord = gl_MultiTexCoord0.xy;
    gl_Position = gl_Vertex;
};


<FS>

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

uniform vec4 params0;
void main()
{
    float width = params0.x;    // * rand( gl_FragCoord );
    float offset = params0.y;
    float d = params0.z;

    float linepos = texCoord.y + d * width * sin( texCoord.x * 3.14159 + offset );
 
    float distance = abs(0.5-linepos);
    float x=(distance/width);

    float intensity = max(1.0-x*x,0.0);

    vec4 color=vec4(0.1,0.1,0.1,1.0)*intensity;
    gl_FragColor=color;
};

