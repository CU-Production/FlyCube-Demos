// The MIT License
// Copyright 2019 Inigo Quilez
// https://www.shadertoy.com/view/wsSGDG

struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer constBuf
{
    float2 iResolution;
    float iTime; // in seconds
    float padding;
};

cbuffer settings
{
    bool bShowRayDirection;
    bool bShowRaymarchingResult;
    bool bShowPos;
    bool bShowNormal;
};

float sdSphere( float3 p, float s )
{
    return length(p) - s;
}

float sdOctahedron(float3 p, float s)
{
    p = abs(p);
    float m = p.x + p.y + p.z - s;
    float3 r = 3.0*p - m;

#if 1
    // filbs111's version (see comments)
    float3 o = min(r, 0.0);
    o = max(r*2.0 - o*3.0 + (o.x+o.y+o.z), 0.0);
    return length(p - s*o/(o.x+o.y+o.z));
#else
    // my original version
	float3 q;
         if( r.x < 0.0 ) q = p.xyz;
    else if( r.y < 0.0 ) q = p.yzx;
    else if( r.z < 0.0 ) q = p.zxy;
    else return m*0.57735027;
    float k = clamp(0.5*(q.z-q.y+s),0.0,s);
    return length(float3(q.x,q.y-s+k,q.z-k));
#endif
}

float map( in float3 pos )
{
//     float rad = 0.1*(0.5+0.5*sin(iTime*2.0));
//     return sdOctahedron(pos,0.5-rad) - rad;

    return sdSphere(pos, 0.5);
}

// http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
float3 calcNormal( in float3 pos )
{
    float2 e = float2(1.0,-1.0)*0.5773;
    const float eps = 0.0005;
    return normalize( e.xyy*map( pos + e.xyy*eps ) +
					  e.yyx*map( pos + e.yyx*eps ) +
					  e.yxy*map( pos + e.yxy*eps ) +
					  e.xxx*map( pos + e.xxx*eps ) );
}

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    // camera movement
	float an = 0.5*(iTime-10.0);
	float3 ro = float3( 1.0*cos(an), -0.4, 1.0*sin(an) ); // rayOrigin
    float3 ta = float3( 0.0, 0.0, 0.0 );
    // camera matrix
    float3 ww = normalize( ta - ro );
    float3 uu = normalize( cross(ww,float3(0.0,1.0,0.0) ) );
    float3 vv = normalize( cross(uu,ww));

    float aspect = iResolution.x / iResolution.y;
    float2 p = float2(aspect, 1.0) * (input.uv * 2.0 - (float2)1.0);
    // create view ray
    float3 rd = normalize(p.x*uu + p.y*vv + 1.5*ww); // rayDirection

    // Debug draw
    if (bShowRayDirection)
        return float4(rd, 1.0);

    // raymarch
    const float tmax = 3.0;
    float t = 0.0;

    for (int i = 0; i < 256; i++) // Max step 256
    {
        float3 pos = ro + t * rd;
        float h = map(pos);
        if ( h < 0.0001 || t > tmax) break;
        t += h;
    }

    // shading/lighting
    float3 col = (float3)0.0;
    if(t < tmax)
    {
        float3 pos = ro + t*rd;
        float3 nor = calcNormal(pos);
        float dif = clamp( dot(nor,float3(0.7,0.6,0.4)), 0.0, 1.0 );
        float amb = 0.5 + 0.5*dot(nor,float3(0.0,0.8,0.6));
        col = float3(0.2,0.3,0.4)*amb + float3(0.8,0.7,0.5)*dif;

        if (bShowRaymarchingResult)
            col = (float3)(t/tmax);
        else if (bShowPos)
            col = pos;
        else if (bShowNormal)
            col = nor;
    }

    // gamma
    col = sqrt( col );

    return float4(col, 1.0);
}
