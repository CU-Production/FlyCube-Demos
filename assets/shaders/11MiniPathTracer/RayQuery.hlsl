// https://nvpro-samples.github.io/vk_mini_path_tracer/index.html
// https://github.com/nvpro-samples/vk_mini_path_tracer/blob/main/vk_mini_path_tracer/shaders/raytrace.comp.glsl

cbuffer resolution
{
    uint screenWidth;
    uint screenHeight;
};

// Limit the kernel to trace at most 64 samples.
#define NUM_SAMPLES 64

// Limit the kernel to trace at most 32 segments.
#define NUM_TRACED_SEGMENTS 32

RWTexture2D<float4> imageData;
RaytracingAccelerationStructure tlas;
StructuredBuffer<float3> vertices;
StructuredBuffer<uint> indices;

// Random number generation using pcg32i_random_t, using inc = 1. Our random state is a uint.
uint stepRNG(uint rngState)
{
    return rngState * 747796405 + 1;
}

// Steps the RNG and returns a floating-point value between 0 and 1 inclusive.
float stepAndOutputRNGFloat(inout uint rngState)
{
    // Condensed version of pcg_output_rxs_m_xs_32_32, with simple conversion to floating-point [0,1].
    rngState  = stepRNG(rngState);
    uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
    word      = (word >> 22) ^ word;
    return float(word) / 4294967295.0f;
}

// Returns the color of the sky in a given direction (in linear color space)
float3 skyColor(float3 direction)
{
    // +y in world space is up, so:
    if(direction.y > 0.0f)
    {
        return lerp(float3(1.0f, 1.0f, 1.0f), float3(0.25f, 0.5f, 1.0f), direction.y);
    }
    else
    {
        return float3(0.03f, 0.03f, 0.03f);
    }
}

struct HitInfo
{
    float3 color;
    float3 worldPosition;
    float3 worldNormal;
};

HitInfo getObjectHitInfo(RayQuery<RAY_FLAG_FORCE_OPAQUE|RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> rayQuery)
{
    HitInfo result;

    // Get the ID of the triangle
    const int primitiveID = rayQuery.CommittedPrimitiveIndex();

    // Get the indices of the vertices of the triangle
    const uint i0 = indices[3 * primitiveID + 0];
    const uint i1 = indices[3 * primitiveID + 1];
    const uint i2 = indices[3 * primitiveID + 2];

    // Get the vertices of the triangle
    const float3 v0 = vertices[i0];
    const float3 v1 = vertices[i1];
    const float3 v2 = vertices[i2];

    // Compute the normal of the triangle in object space, using the right-hand
    // rule. Since our transformation matrix is the identity, object space
    // is the same as world space.
    //    v2       .
    //    |\       .
    //    | \      .
    //    |  \     .
    //    |/  \    .
    //    /    \   .
    //   /|     \  .
    //  L v0----v1 .
    // n
    //const float3 objectNormal = normalize(cross(v1 - v0, v2 - v0));
    const float3 objectNormal = normalize(cross(v2 - v0, v1 - v0));

    // Get the barycentric coordinates of the intersection
    float3 barycentrics = float3(0.0, rayQuery.CommittedTriangleBarycentrics());
    barycentrics.x    = 1.0 - barycentrics.y - barycentrics.z;

    // Compute the coordinates of the intersection
    const float3 objectPos = v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
    // For the main tutorial, object space is the same as world space:
    result.worldPosition = objectPos;

    // For the main tutorial, object space is the same as world space:
    result.worldNormal = objectNormal;

    result.color = float3(0.8f, 0.8f, 0.8f);

    const float dotX = dot(result.worldNormal, float3(1.0, 0.0, 0.0));
    if(dotX > 0.99)
    {
        result.color = float3(0.8, 0.0, 0.0);
    }
    else if(dotX < -0.99)
    {
        result.color = float3(0.0, 0.8, 0.0);
    }

    return result;
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

    // State of the random number generator.
    uint rngState = resolution.x * pixel.y + pixel.x;  // Initial seed

    // This scene uses a right-handed coordinate system like the OBJ file format, where the
    // +x axis points right, the +y axis points up, and the -z axis points into the screen.
    // The camera is located at (-0.001, 1, 6).
    const float3 cameraOrigin = float3(-0.001f, 1.0f, 6.0f);
    // Define the field of view by the vertical slope of the topmost rays:
    const float fovVerticalSlope = 1.0 / 5.0;

    float3 summedPixelColor = float3(0.0, 0.0, 0.0);

    // Limit the kernel to trace at most 64 samples.
    for (int sampleIdx = 0; sampleIdx < NUM_SAMPLES; sampleIdx++)
    {
        // Rays always originate at the camera for now. In the future, they'll
        // bounce around the scene.
        float3 rayOrigin = cameraOrigin;

        // Compute the direction of the ray for this pixel. To do this, we first
        // transform the screen coordinates to look like this, where a is the
        // aspect ratio (width/height) of the screen:
        //           1
        //    .------+------.
        //    |      |      |
        // -a + ---- 0 ---- + a
        //    |      |      |
        //    '------+------'
        //          -1
        const float2 randomPixelCenter = float2(pixel) + float2(stepAndOutputRNGFloat(rngState), stepAndOutputRNGFloat(rngState));
        const float2 screenUV          = float2((2.0 * randomPixelCenter.x - resolution.x) / resolution.y,    //
                                               -(2.0 * randomPixelCenter.y - resolution.y) / resolution.y);  // Flip the y axis

        // Next, define the field of view by the vertical slope of the topmost rays,
        // and create a ray direction:
        float3 rayDirection = float3(fovVerticalSlope * screenUV.x, fovVerticalSlope * screenUV.y, -1.0);
        rayDirection = normalize(rayDirection);

        float3 accumulatedRayColor = float3(1.0, 1.0, 1.0);  // The amount of light that made it to the end of the current ray.


        // Limit the kernel to trace at most 32 segments.
        for(int tracedSegments = 0; tracedSegments < NUM_TRACED_SEGMENTS; tracedSegments++)
        {
            RayDesc rayDesc;
            rayDesc.Origin = rayOrigin;        // Ray origin
            rayDesc.Direction = rayDirection;  // Ray direction
            rayDesc.TMin = 0.001;              // Minimum t-value
            rayDesc.TMax = 10000.0;            // Maximum t-value

            RayQuery<RAY_FLAG_FORCE_OPAQUE|RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> rayQuery;
            rayQuery.TraceRayInline(tlas,                  // Top-level acceleration structure
                                    RAY_FLAG_FORCE_OPAQUE, // Ray flags, https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#ray-flags
                                    0xFF,                  // 8-bit instance mask, here saying "trace against all instances"
                                    rayDesc);

            // Start traversal, and loop over all ray-scene intersections. When this finishes,
            // rayQuery stores a "committed" intersection, the closest intersection (if any).
            while(rayQuery.Proceed())
            {
            }

            // Get the type of committed (true) intersection - nothing, a triangle, or
            // a generated object
            if(rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
            {
                // Ray hit a triangle
                HitInfo hitInfo = getObjectHitInfo(rayQuery);

                // Apply color absorption
                accumulatedRayColor *= hitInfo.color;

                // Flip the normal so it points against the ray direction:
                hitInfo.worldNormal = faceforward(hitInfo.worldNormal, rayDirection, hitInfo.worldNormal);

                // Start a new ray at the hit position, but offset it slightly along the normal:
                rayOrigin = hitInfo.worldPosition + 0.0001 * hitInfo.worldNormal;

                // For a random diffuse bounce direction, we follow the approach of
                // Ray Tracing in One Weekend, and generate a random point on a sphere
                // of radius 1 centered at the normal. This uses the random_unit_vector
                // function from chapter 8.5:
                const float theta = 6.2831853 * stepAndOutputRNGFloat(rngState);   // Random in [0, 2pi]
                const float u     = 2.0 * stepAndOutputRNGFloat(rngState) - 1.0;  // Random in [-1, 1]
                const float r     = sqrt(1.0 - u * u);
                rayDirection      = hitInfo.worldNormal + float3(r * cos(theta), r * sin(theta), u);
                // Then normalize the ray direction:
                rayDirection = normalize(rayDirection);
            }
            else
            {
                // Ray hit the sky
                accumulatedRayColor *= skyColor(rayDirection);

                // Sum this with the pixel's other samples.
                // (Note that we treat a ray that didn't find a light source as if it had
                // an accumulated color of (0, 0, 0)).
                summedPixelColor += accumulatedRayColor;

                break;
            }
        }

    }

    // Give the pixel the color:
    imageData[pixel] = float4(summedPixelColor / float(NUM_SAMPLES), 1.0f);

    // gamma
    imageData[pixel] = float4(pow(abs(imageData[pixel].xyz), 1 / 2.2), 1.0f);
}