// https://github.com/MomoDeve/PathTracer/blob/master/Resources/path_tracing.glsl

cbuffer resolution
{
    uint screenWidth;
    uint screenHeight;
};

struct Material
{
    float3 emmitance;
    float3 reflectance;
    float roughness;
    float opacity;
};

struct Box
{
    Material material;
    float3 halfSize;
    float3x3 rotation;
    float3 position;
};

struct Sphere
{
    Material material;
    float3 position;
    float radius;
};

cbuffer uniformBlock
{
    float3 uPosition;
    float uTime;
    int uSamples;
};

#define PI 3.1415926535
#define HALF_PI (PI / 2.0)
#define FAR_DISTANCE 1000000.0

#define MAX_DEPTH 8
#define SPHERE_COUNT 3
#define BOX_COUNT 8
#define N_IN 0.99
#define N_OUT 1.0

//Sphere spheres[SPHERE_COUNT];
//Box boxes[BOX_COUNT];
RWTexture2D<float4> imageData;
RWStructuredBuffer<Sphere> spheres;
RWStructuredBuffer<Box> boxes;

#define vec2 float2
#define vec3 float3
#define vec4 float4
#define mat3 float3x3
#define mat4 float4x4
#define fract frac
#define mix lerp


void InitializeScene()
{
    spheres[0].position = vec3(2.5, 1.5, -1.5);
    spheres[1].position = vec3(-2.5, 2.5, -1.0);
    spheres[2].position = vec3(0.5, -4.0, 3.0);
    spheres[0].radius = 1.5;
    spheres[1].radius = 1.0;
    spheres[2].radius = 1.0;
    spheres[0].material.roughness = 1.0;
    spheres[1].material.roughness = 0.8;
    spheres[2].material.roughness = 1.0;
    spheres[0].material.opacity = 0.0;
    spheres[1].material.opacity = 0.0;
    spheres[2].material.opacity = 0.8;
    spheres[0].material.reflectance = vec3(1.0, 0.0, 0.0);
    spheres[1].material.reflectance = vec3(1.0, 0.4, 0.0);
    spheres[2].material.reflectance = vec3(1.0, 1.0, 1.0);
    spheres[0].material.emmitance = vec3(0.0, 0.0, 0.0);
    spheres[1].material.emmitance = vec3(0.0, 0.0, 0.0);
    spheres[2].material.emmitance = vec3(0.0, 0.0, 0.0);

    // up
    boxes[0].material.roughness = 0.0;
    boxes[0].material.emmitance = vec3(0.0, 0.0, 0.0);
    boxes[0].material.reflectance = vec3(1.0, 1.0, 1.0);
    boxes[0].halfSize = vec3(5.0, 0.5, 5.0);
    boxes[0].position = vec3(0.0, 5.5, 0.0);
    boxes[0].rotation = mat3(
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0
    );

    // down
    boxes[1].material.roughness = 0.3;
    boxes[1].material.opacity = 0.0;
    boxes[1].material.emmitance = vec3(0.0, 0.0, 0.0);
    boxes[1].material.reflectance = vec3(1.0, 1.0, 1.0);
    boxes[1].halfSize = vec3(5.0, 0.5, 5.0);
    boxes[1].position = vec3(0.0, -5.5, 0.0);
    boxes[1].rotation = mat3(
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0
    );

    // right
    boxes[2].material.roughness = 0.0;
    boxes[2].material.opacity = 0.0;
    boxes[2].material.emmitance = vec3(0.0, 0.0, 0.0);
    boxes[2].material.reflectance = vec3(0.0, 1.0, 0.0);
    boxes[2].halfSize = vec3(5.0, 0.5, 5.0);
    boxes[2].position = vec3(5.5, 0.0, 0.0);
    boxes[2].rotation = mat3(
            0.0, 1.0, 0.0,
            -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0
    );

    // left
    boxes[3].material.roughness = 0.0;
    boxes[3].material.opacity = 0.0;
    boxes[3].material.emmitance = vec3(0.0, 0.0, 0.0);
    boxes[3].material.reflectance = vec3(1.0, 0.0, 0.0);
    boxes[3].halfSize = vec3(5.0, 0.5, 5.0);
    boxes[3].position = vec3(-5.5, 0.0, 0.0);
    boxes[3].rotation = mat3(
            0.0, 1.0, 0.0,
            -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0
    );

    // back
    boxes[4].material.roughness = 0.0;
    boxes[4].material.opacity = 0.0;
    boxes[4].material.emmitance = vec3(0.0, 0.0, 0.0);
    boxes[4].material.reflectance = vec3(1.0, 1.0, 1.0);
    boxes[4].halfSize = vec3(5.0, 0.5, 5.0);
    boxes[4].position = vec3(0.0, 0.0, -5.5);
    boxes[4].rotation = mat3(
            1.0, 0.0, 0.0,
            0.0, 0.0, 1.0,
            0.0, 1.0, 0.0
    );

    // light source
    boxes[5].material.roughness = 0.0;
    boxes[5].material.opacity = 0.0;
    boxes[5].material.emmitance = vec3(6.0, 6.0, 6.0);
    boxes[5].material.reflectance = vec3(1.0, 1.0, 1.0);
    boxes[5].halfSize = vec3(2.5, 0.2, 2.5);
    boxes[5].position = vec3(0.0, 4.8, 0.0);
    boxes[5].rotation = mat3(
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0
    );

    // boxes
    boxes[6].material.roughness = 0.0;
    boxes[6].material.opacity = 0.0;
    boxes[6].material.emmitance = vec3(0.0, 0.0, 0.0);
    boxes[6].material.reflectance = vec3(1.0, 1.0, 1.0);
    boxes[6].halfSize = vec3(1.5, 3.0, 1.5);
    boxes[6].position = vec3(-2.0, -2.0, -0.0);
    boxes[6].rotation = mat3(
            0.7, 0.0, 0.7,
            0.0, 1.0, 0.0,
            -0.7, 0.0, 0.7
    );
    // boxes
    boxes[7].material.roughness = 0.0;
    boxes[7].material.opacity = 0.0;
    boxes[7].material.emmitance = vec3(0.0, 0.0, 0.0);
    boxes[7].material.reflectance = vec3(1.0, 1.0, 1.0);
    boxes[7].halfSize = vec3(1.0, 1.5, 1.0);
    boxes[7].position = vec3(2.5, -3.5, -0.0);
    boxes[7].rotation = mat3(
            0.7, 0.0, 0.7,
            0.0, 1.0, 0.0,
            -0.7, 0.0, 0.7
    );
}

float RandomNoise(vec2 co)
{
    co *= fract(uTime * 12.343);
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 RandomHemispherePoint(vec2 rand)
{
    float cosTheta = sqrt(1.0 - rand.x);
    float sinTheta = sqrt(rand.x);
    float phi = 2.0 * PI * rand.y;
    return vec3(
            cos(phi) * sinTheta,
            sin(phi) * sinTheta,
            cosTheta
    );
}

vec3 NormalOrientedHemispherePoint(vec2 rand, vec3 n)
{
    vec3 v = RandomHemispherePoint(rand);
    return dot(v, n) < 0.0 ? -v : v;
}

float FresnelSchlick(float nIn, float nOut, vec3 direction, vec3 normal)
{
    float R0 = ((nOut - nIn) * (nOut - nIn)) / ((nOut + nIn) * (nOut + nIn));
    float fresnel = R0 + (1.0 - R0) * pow((1.0 - abs(dot(direction, normal))), 5.0);
    return fresnel;
}

vec3 IdealRefract(vec3 direction, vec3 normal, float nIn, float nOut)
{
    bool fromOutside = dot(normal, direction) < 0.0;
    float ratio = fromOutside ? nOut / nIn : nIn / nOut;

    vec3 refraction, reflection;

    refraction = fromOutside ? refract(direction, normal, ratio) : -refract(-direction, normal, ratio);
    reflection = reflect(direction, normal);

    return refraction == vec3(0.0, 0.0, 0.0) ? reflection : refraction;
}

bool IntersectRaySphere(vec3 origin, vec3 direction, Sphere sphere, out float fraction, out vec3 normal)
{
    vec3 L = origin - sphere.position;
    float a = dot(direction, direction);
    float b = 2.0 * dot(L, direction);
    float c = dot(L, L) - sphere.radius * sphere.radius;
    float D = b * b - 4 * a * c;

    if (D < 0.0) return false;

    float r1 = (-b - sqrt(D)) / (2.0 * a);
    float r2 = (-b + sqrt(D)) / (2.0 * a);

    if (r1 > 0.0)
        fraction = r1;
    else if (r2 > 0.0)
        fraction = r2;
    else
        return false;

    normal = normalize(direction * fraction + L);

    return true;
}

bool IntersectRayBox(vec3 origin, vec3 direction, Box box, out float fraction, out vec3 normal)
{
    vec3 rd = mul(direction, box.rotation);
    vec3 ro = mul((origin - box.position), box.rotation);

    vec3 m = vec3(1.0, 1.0, 1.0) / rd;

    vec3 s = vec3((rd.x < 0.0) ? 1.0 : -1.0,
                  (rd.y < 0.0) ? 1.0 : -1.0,
                  (rd.z < 0.0) ? 1.0 : -1.0);
    vec3 t1 = m * (-ro + s * box.halfSize);
    vec3 t2 = m * (-ro - s * box.halfSize);

    float tN = max(max(t1.x, t1.y), t1.z);
    float tF = min(min(t2.x, t2.y), t2.z);

    if (tN > tF || tF < 0.0) return false;

    mat3 txi = transpose(box.rotation);

    if (t1.x > t1.y && t1.x > t1.z)
        normal = txi[0] * s.x;
    else if (t1.y > t1.z)
        normal = txi[1] * s.y;
    else
        normal = txi[2] * s.z;

    fraction = tN;

    return true;
}

bool CastRay(vec3 rayOrigin, vec3 rayDirection, out float fraction, out vec3 normal, out Material material)
{
    float minDistance = FAR_DISTANCE;

    for (int i = 0; i < SPHERE_COUNT; i++)
    {
        float F;
        vec3 N;
        if (IntersectRaySphere(rayOrigin, rayDirection, spheres[i], F, N) && F < minDistance)
        {
            minDistance = F;
            normal = N;
            material = spheres[i].material;
        }
    }

    for (int i = 0; i < BOX_COUNT; i++)
    {
        float F;
        vec3 N;
        if (IntersectRayBox(rayOrigin, rayDirection, boxes[i], F, N) && F < minDistance)
        {
            minDistance = F;
            normal = N;
            material = boxes[i].material;
        }
    }

    fraction = minDistance;
    return minDistance != FAR_DISTANCE;
}

bool IsRefracted(float rand, vec3 direction, vec3 normal, float opacity, float nIn, float nOut)
{
    float fresnel = FresnelSchlick(nIn, nOut, direction, normal);
    return opacity > rand && fresnel < rand;
}

vec3 TracePath(vec3 rayOrigin, vec3 rayDirection, float seed, vec2 TexCoord)
{
    vec3 L = vec3(0.0, 0.0, 0.0);
    vec3 F = vec3(1.0, 1.0, 1.0);
    for (int i = 0; i < MAX_DEPTH; i++)
    {
        float fraction;
        vec3 normal;
        Material material;
        bool hit = CastRay(rayOrigin, rayDirection, fraction, normal, material);
        if (hit)
        {
            vec3 newRayOrigin = rayOrigin + fraction * rayDirection;

            vec2 rand = vec2(RandomNoise(seed * TexCoord.xy), seed * RandomNoise(TexCoord.yx));
            vec3 hemisphereDistributedDirection = NormalOrientedHemispherePoint(rand, normal);

            vec3 randomVec = vec3(
                    RandomNoise(sin(seed * TexCoord.xy)),
                    RandomNoise(cos(seed * TexCoord.xy)),
                    RandomNoise(sin(seed * TexCoord.yx))
            );
            randomVec = normalize(2.0 * randomVec - 1.0);

            vec3 tangent = cross(randomVec, normal);
            vec3 bitangent = cross(normal, tangent);
            mat3 transform = mat3(tangent, bitangent, normal);

            vec3 newRayDirection = mul(hemisphereDistributedDirection, transform);

            float refractRand = RandomNoise(cos(seed * TexCoord.yx));
            bool refracted = IsRefracted(refractRand, rayDirection, normal, material.opacity, N_IN, N_OUT);
            if (refracted)
            {
                vec3 idealRefraction = IdealRefract(rayDirection, normal, N_IN, N_OUT);
                newRayDirection = normalize(mix(-newRayDirection, idealRefraction, material.roughness));
                newRayOrigin += normal * (dot(newRayDirection, normal) < 0.0 ? -0.8 : 0.8);
            }
            else
            {
                vec3 idealReflection = reflect(rayDirection, normal);
                newRayDirection = normalize(mix(newRayDirection, idealReflection, material.roughness));
                newRayOrigin += normal * 0.8;
            }

            rayDirection = newRayDirection;
            rayOrigin = newRayOrigin;

            L += F * material.emmitance;
            F *= material.reflectance;
        }
        else
        {
            F = vec3(1.0, 1.0, 1.0);
        }
    }

    return L;
}

[numthreads(8, 8, 1)]
void MainCS(uint3 Gid  : SV_GroupID,          // current group index (dispatched by c++)
            uint3 DTid : SV_DispatchThreadID, // "global" thread id
            uint3 GTid : SV_GroupThreadID,    // current threadId in group / "local" threadId
            uint GI    : SV_GroupIndex)       // "flattened" index of a thread within a group)
{
    // The resolution of the buffer, which in this case is a hardcoded vector
    // of 2 unsigned integers:
    const uint2 resolution = uint2(screenWidth, screenHeight);

    // Get the coordinates of the pixel for this invocation:
    //
    // .-------.-> x
    // |       |
    // |       |
    // '-------'
    // v
    // y
    const uint2 pixel = DTid.xy;

    // If the pixel is outside of the image, don't do anything:
    if((pixel.x >= resolution.x) || (pixel.y >= resolution.y))
    {
        return;
    }

    vec2 TexCoord = (vec2)DTid.xy / (vec2)resolution;

    if (GTid.x == 0 && GTid.y == 0 && GTid.z == 0)
    {
        InitializeScene();
    }
    AllMemoryBarrierWithGroupSync();

    const float fovVerticalSlope = 1.0 / 5.0;
    const float2 randomPixelCenter = float2(pixel);
    const float2 screenUV          = float2((2.0 * randomPixelCenter.x - resolution.x) / resolution.y,    //
                                           -(2.0 * randomPixelCenter.y - resolution.y) / resolution.y);
    float3 rayDirection = float3(fovVerticalSlope * screenUV.x, fovVerticalSlope * screenUV.y, -1.0);
    rayDirection = normalize(rayDirection);
    vec3 direction = rayDirection;

    vec3 totalColor = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < uSamples; i++)
    {
        float seed = sin(float(i) * uTime);
        vec3 sampleColor = TracePath(uPosition, direction, seed, TexCoord);
        totalColor += sampleColor;
    }

    vec3 color = totalColor / float(uSamples);
    color = color / (color + vec3(1.0, 1.0, 1.0));
    float tmp = 1.0 / 2.2;
    color = pow(color, vec3(tmp, tmp, tmp));
    vec4 OutColor = vec4(color, 1.0);
    imageData[pixel] = OutColor;
}