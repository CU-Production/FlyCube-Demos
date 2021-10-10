// https://docs.unity3d.com/Manual/class-Texture3D.html

// Maximum amount of raymarching samples
#define MAX_STEP_COUNT 128

// Allowed floating point inaccuracy
#define EPSILON 0.00001f

struct VS_OUTPUT
{
    float4 vertex : SV_POSITION;
    float3 objectVertex : TEXCOORD0;
    float3 vectorToSurface : TEXCOORD1;
};

cbuffer Constants
{
    float4x4 WorldToObj;
};

cbuffer Settings
{
    float volumeAlpha;
    float stepSize;
    int maxStepCount;
};

Texture3D volumeTexture;
// Texture2D volumeSlicesTexture;
SamplerState g_sampler;

float min3(float a, float b, float c)
{
    return min(a, min(b, c));
}

float max3(float a, float b, float c)
{
    return max(a, max(b, c));
}

float4 BlendUnder(float4 color, float4 newColor)
{
    color.rgb += (1.0 - color.a) * newColor.a * newColor.rgb;
    color.a += (1.0 - color.a) * newColor.a;
    return color;
}

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    // Start raymarching at the front surface of the object
    float3 rayOrigin = input.objectVertex;

    // Use vector from camera to object surface to get ray direction
    float3 rayDirection = mul(float4(normalize(input.vectorToSurface), 1), WorldToObj).xyz;

    float4 color = float4(0, 0, 0, 0);
    float3 samplePosition = rayOrigin;

    // Raymarch through object space
    [loop]
    for (int i = 0; i < maxStepCount; i++)
    {
        // Accumulate color only within unit cube bounds
        if(max3(abs(samplePosition.x), abs(samplePosition.y), abs(samplePosition.z)) < 0.5f + EPSILON)
        {
            float4 sampledColor = volumeTexture.Sample(g_sampler, samplePosition + float3(0.5f, 0.5f, 0.5f));
            sampledColor.a *= volumeAlpha;
            color = BlendUnder(color, sampledColor);
            samplePosition += rayDirection * stepSize;
        }
        else
        {
            break;
        }
    }

    // Gamma
    color.rgb = pow( color.rgb, 1.0f/2.2f );

    return color;
}
