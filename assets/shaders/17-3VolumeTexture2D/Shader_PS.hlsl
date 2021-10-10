// https://docs.unity3d.com/Manual/class-Texture3D.html

// Maximum amount of raymarching samples
#define MAX_STEP_COUNT 128

// Allowed floating point inaccuracy
#define EPSILON 0.00001f

const static int x_slices = 16;
const static int y_slices = 8;
const static int tex_width = 2048;
const static int tex_height = 1024;
const static int tex3d_size = 128;

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
    bool bUsingZFilter;
};

Texture2D volumeSlicesTexture;
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
            float3 uvin3D = samplePosition + float3(0.5f, 0.5f, 0.5f);
            uvin3D.z += 0.1f; // Move back
            float2 uvOffsetInSlice = uvin3D.xy / float2((float)x_slices, (float)y_slices);
            uvOffsetInSlice.y = 1.0 - uvOffsetInSlice.y; // FlipY
            int uvZ = floor(uvin3D.z * tex3d_size);

            int g_x = uvZ % x_slices;
            int g_y = uvZ / x_slices;
            float2 ampleLocation2D = float2((float)g_x * tex3d_size, (float)g_y * tex3d_size) / float2((float)tex_width, (float)tex_height);
            ampleLocation2D += uvOffsetInSlice;

            float4 sampledColor = 0.0;
            [branch]
            if (bUsingZFilter)
            {
                int uvZ2 = uvZ +1;
                float factor = uvin3D.z * tex3d_size - uvZ;

                int g2_x = uvZ % x_slices;
                int g2_y = uvZ / x_slices;
                float2 ampleLocation2D2 = float2((float)g2_x * tex3d_size, (float)g2_y * tex3d_size) / float2((float)tex_width, (float)tex_height);
                ampleLocation2D2 += uvOffsetInSlice;

                float4 color1 = volumeSlicesTexture.Sample(g_sampler, ampleLocation2D);
                float4 color2 = volumeSlicesTexture.Sample(g_sampler, ampleLocation2D2);
                sampledColor = lerp(color1, color2, factor);
            }
            else
            {
                sampledColor = volumeSlicesTexture.Sample(g_sampler, ampleLocation2D);
            }
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
