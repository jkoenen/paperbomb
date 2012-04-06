<Shared>
varying vec2 texCoord;

<VS>
vec3 flipPageVertex( vec2 vi, float A, float theta, float rho )
{
    float R     = sqrt(vi.x * vi.x + pow(vi.y - A, 2.0f));  // Radius of the circle circumscribed by vertex (vi.x, vi.y) around A on the x-y plane.
    float r     = R * sin(theta);                       // From R, calculate the radius of the cone cross section intersected by our vertex in 3D space.
    float beta  = asin(vi.x / R) / sin(theta);          // Angle SCT, the angle of the cone cross section subtended by the arc |ST|.
    
    vec3 v1;
    v1.x  = r * sin(beta);
    v1.y  = R + A - r * (1.0f - cos(beta)) * sin(theta); // *** MAGIC!!! ***
    v1.z  = r * (1.0f - cos(beta)) * cos(theta);

    // Apply a basic rotation transform around the y axis to rotate the curled page. These two steps could be combined
    // through simple substitution, but are left separate to keep the math simple for debugging and illustrative purposes.
    vec3 vo;
    vo.x = (v1.x * cos(rho) - v1.z * sin(rho));
    vo.y =  v1.y;
    vo.z = (v1.x * sin(rho) + v1.z * cos(rho));
    
    
    return vec3(vo.y,1.0-vo.x,vo.z);
}

uniform vec4 vp0;
void main()
{
    float A = vp0.x;
    float theta = vp0.y; 
    float rho = vp0.z;

    vec2 inputPos = vec2(0.5,0.5)*gl_Vertex.xy+vec2(0.5,0.5);

    vec3 flippedPos = flipPageVertex( inputPos, A, theta, rho );

    // todo: project 3d to 2d coordinate:
    vec4 clipPos=vec4(vec2(2.0,2.0)*flippedPos.xy-vec2(1.0,1.0),0.0f,1.0f);

    texCoord = gl_MultiTexCoord0.xy;
    texCoord = vec2(texCoord.y,texCoord.x);
    vec4 pos = gl_Vertex;
    gl_Position = clipPos;
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

