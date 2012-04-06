<Shared>
varying vec2 texCoord;
varying vec2 paperPos;

<VS>

void main()
{
    texCoord = gl_MultiTexCoord0.xy;

    vec2 paperSize = vec2( 64.0, 36.0 );

    paperPos = gl_Vertex.xy;
    vec2 clipPos = vec2(2.0f*paperPos.x/paperSize.x,2.0f*(paperSize.y-paperPos.y)/paperSize.y)-vec2(1.0f,1.0f);

    gl_Position = vec4(clipPos,0.0f,1.0f);
}

<FS>

float distToLine(vec2 p0, vec2 p1, vec2 p)
{
    vec2 dir=p1-p0;
    float l2=dot(dir,dir);
    float t=dot(p-p0,dir)/l2;
    if(t<0.0)
    {
        return distance(p,p0);
    }
    else if(t>1.0)
    {
        return distance(p,p1);
    }
    else
    {
        vec2 proj=p0+t*dir;
        return distance(p,proj);
    }
}

uniform vec4 fp0;
uniform vec4 fp1;
uniform sampler2D ft0;

void main()
{
    vec4 noise0=vec4(2.0,2.0,2.0,2.0)*texture2D(ft0,texCoord)-vec4(1.0,1.0,1.0,1.0);
    vec4 noise1=vec4(2.0,2.0,2.0,2.0)*texture2D(ft0,vec2(1.3,1.2)*texCoord.yx)-vec4(1.0,1.0,1.0,1.0);
    vec4 noise=noise0+noise1;

    vec2 start=fp0.xy;
    vec2 end=fp0.zw;
    float size=fp1.x;
    float n=(noise.x+0.5*noise.y+0.25*noise.z+0.125*noise.w);
    float d=distToLine(start,end,paperPos)+n;
    float i=max(min((size-d),1),0);
    gl_FragColor=vec4(i,i,i,i);
}

