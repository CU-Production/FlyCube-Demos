#include <catch2/catch_all.hpp>

#include <ShaderReflection/ShaderReflection.h>
#include <HLSLCompiler/Compiler.h>

class ShaderTestCase
{
public:
    virtual const ShaderDesc& GetShaderDesc() const = 0;
    virtual void Test(ShaderBlobType type, const void* data, size_t size) const = 0;
};

void RunTest(const ShaderTestCase& test_case)
{
#ifdef DIRECTX_SUPPORT
    auto dxil_blob = Compile(test_case.GetShaderDesc(), ShaderBlobType::kDXIL);
    REQUIRE(!dxil_blob.empty());
    test_case.Test(ShaderBlobType::kDXIL, dxil_blob.data(), dxil_blob.size());
#endif

#ifdef VULKAN_SUPPORT
    auto spirv_blob = Compile(test_case.GetShaderDesc(), ShaderBlobType::kSPIRV);
    REQUIRE(!spirv_blob.empty());
    test_case.Test(ShaderBlobType::kSPIRV, spirv_blob.data(), spirv_blob.size() * sizeof(spirv_blob.front()));
#endif
}

class RayTracing : public ShaderTestCase
{
public:
    const ShaderDesc& GetShaderDesc() const override
    {
        return m_desc;
    }

    void Test(ShaderBlobType type, const void* data, size_t size) const
    {
        REQUIRE(data);
        REQUIRE(size);
        auto reflection = CreateShaderReflection(type, data, size);
        REQUIRE(reflection);

        std::vector<EntryPoint> expect = {
            { "ray_gen", ShaderKind::kRayGeneration },
            { "miss",    ShaderKind::kMiss },
            { "closest", ShaderKind::kClosestHit },
        };
        auto entry_points = reflection->GetEntryPoints();
        sort(entry_points.begin(), entry_points.end());
        sort(expect.begin(), expect.end());
        REQUIRE(entry_points.size() == expect.size());
        for (size_t i = 0; i < entry_points.size(); ++i)
        {
            REQUIRE(entry_points[i].name == expect[i].name);
            REQUIRE(entry_points[i].kind == expect[i].kind);
        }
    }

private:
    ShaderDesc m_desc = { ASSETS_PATH"shaders/DxrTriangle/RayTracing.hlsl", "", ShaderType::kLibrary, "6_3" };
};

class TrianglePS : public ShaderTestCase
{
public:
    const ShaderDesc& GetShaderDesc() const override
    {
        return m_desc;
    }

    void Test(ShaderBlobType type, const void* data, size_t size) const
    {
        REQUIRE(data);
        REQUIRE(size);
        auto reflection = CreateShaderReflection(type, data, size);
        REQUIRE(reflection);

        std::vector<EntryPoint> expect = {
            { "main", ShaderKind::kPixel },
        };
        auto entry_points = reflection->GetEntryPoints();
        REQUIRE(entry_points == expect);

        auto bindings = reflection->GetBindings();
        REQUIRE(bindings.size() == 1);
        REQUIRE(bindings.front().name == "Settings");
    }

private:
    ShaderDesc m_desc = { ASSETS_PATH"shaders/Triangle/PixelShader.hlsl", "main", ShaderType::kPixel, "6_3" };
};

class TriangleVS : public ShaderTestCase
{
public:
    const ShaderDesc& GetShaderDesc() const override
    {
        return m_desc;
    }

    void Test(ShaderBlobType type, const void* data, size_t size) const
    {
        REQUIRE(data);
        REQUIRE(size);
        auto reflection = CreateShaderReflection(type, data, size);
        REQUIRE(reflection);

        std::vector<EntryPoint> expect = {
            { "main", ShaderKind::kVertex },
        };
        auto entry_points = reflection->GetEntryPoints();
        REQUIRE(entry_points == expect);
    }

private:
    ShaderDesc m_desc = { ASSETS_PATH"shaders/Triangle/VertexShader.hlsl", "main", ShaderType::kVertex, "6_3" };
};

TEST_CASE("ShaderReflection")
{
    RunTest(RayTracing{});
    RunTest(TrianglePS{});
    RunTest(TriangleVS{});
}
