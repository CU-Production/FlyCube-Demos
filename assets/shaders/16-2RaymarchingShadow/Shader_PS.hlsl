// The MIT License
// Copyright 2019 Inigo Quilez
// https://www.shadertoy.com/view/wsSGDG
// https://iquilezles.org/www/articles/distfunctions/distfunctions.htm

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
    bool bUseShadow;
    float softShadowBias;
};

float sdSphere( float3 p, float s )
{
    return length(p) - s;
}

float sdPlane( float3 p, float h )
{
  // n must be normalized
  return p.y + h;
}

float sdPlane( float3 p, float3 n, float h )
{
  // n must be normalized
  return dot(p,n) + h;
}

float2 opU( float2 d1, float2 d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}

float map( in float3 pos )
{
//     return sdSphere(pos, 0.5);
//     return sdPlane(pos, float3(0.0, -1.0, 0.0), 0.5);
    float2 d1 = float2(sdSphere(pos, 0.5), 1.0);
    float2 d2 = float2(sdPlane(pos, 0.5), 1.0);
    return opU(d1, d2).x;
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

float softShadow ( in float3 ro, in float3 rd )
{
    float res = 1.0;

    [loop]
    for (float t = 0.1; t < 8.0;)
    {
        float h = map(ro + t * rd);
        if (h < 0.001) return 0.0;
        res = min(res, softShadowBias * h/t);
        t += h;
    }
    return res;
}

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    // camera movement
	float an = 0.5*(iTime-10.0);
	float3 ro = float3( 1.0*cos(an), 0.4, 1.0*sin(an) ); // rayOrigin
    float3 ta = float3( 0.0, 0.0, 0.0 );
    // camera matrix
    float3 ww = normalize( ta - ro );
    float3 uu = normalize( cross(ww,float3(0.0,1.0,0.0) ) );
    float3 vv = normalize( cross(uu,ww));

    float aspect = iResolution.x / iResolution.y;
    input.uv.y = 1.0 - input.uv.y; // HLSL need flip Y
    float2 p = float2(aspect, 1.0) * (input.uv * 2.0 - (float2)1.0);
    // create view ray
    float3 rd = normalize(p.x*uu + p.y*vv + 1.5*ww); // rayDirection

    // Debug draw
    if (bShowRayDirection)
        return float4(rd, 1.0);

    // raymarch
    const float tmax = 10.0;
    float t = 0.0;

    [loop]
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
        float3 light = float3(0.7,0.6,0.4);
        float dif = clamp( dot(nor,light), 0.0, 1.0 );

        if (bUseShadow)
        {
            float shadow = softShadow(pos, light);
            dif *= shadow;
        }

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
