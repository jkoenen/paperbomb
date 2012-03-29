<VS>
//uniform vec4 p0[4];
void main()
{
    gl_Position=gl_Vertex;
};


<FS>
uniform vec4 c0;
void main()
{
    gl_FragColor = vec4(c0.xyz,1);
};

