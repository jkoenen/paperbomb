<Shared>

<VS>

void main()
{
    vec2 paperSize = vec2( 64.0, 36.0 );

    vec2 paperPos = gl_Vertex.xy;
    vec2 clipPos = vec2(2.0f*paperPos.x/paperSize.x,2.0f*(paperSize.y-paperPos.y)/paperSize.y)-vec2(1.0f,1.0f);

    gl_Position = vec4(clipPos,0.0f,1.0f);
}

<FS>

uniform vec4 fp0;
void main()
{
    vec4 penColor=fp0.xyzw;
    gl_FragColor=penColor;
}

