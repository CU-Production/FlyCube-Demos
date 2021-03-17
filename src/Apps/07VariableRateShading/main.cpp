#include "AppBox/AppBox.h"
#include "AppBox/ArgsParser.h"
#include "RenderDevice/RenderDevice.h"
#include "Texture/TextureLoader.h"
#include "Utilities/FormatHelper.h"
#include <stdexcept>

#include "ProgramRef/ShaderPS.h"
#include "ProgramRef/ShaderVS.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("07 - VariableRateShading", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    if (!device->IsVariableRateShadingSupported())
        throw std::runtime_error("Variable rate shading is not supported");
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();
    std::vector<uint32_t> ibuf = { 0, 1, 2, 0, 2, 3 };
    std::shared_ptr<Resource> index = device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * ibuf.size());
    upload_command_list->UpdateSubresource(index, 0, ibuf.data(), 0, 0);
    std::vector<glm::vec3> pbuf = {
            glm::vec3(-0.5,  0.5, 0.0),
            glm::vec3( 0.5,  0.5, 0.0),
            glm::vec3( 0.5, -0.5, 0.0),
            glm::vec3(-0.5, -0.5, 0.0),
    };
    std::vector<glm::vec2> uvbuf = {
            glm::vec2(0.0, 0.0),
            glm::vec2(1.0, 0.0),
            glm::vec2(1.0, 1.0),
            glm::vec2(0.0, 1.0),
    };
    std::shared_ptr<Resource> pos = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec3) * pbuf.size());
    upload_command_list->UpdateSubresource(pos, 0, pbuf.data(), 0, 0);
    std::shared_ptr<Resource> uv = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec2) * uvbuf.size());
    upload_command_list->UpdateSubresource(uv, 0, uvbuf.data(), 0, 0);

    std::shared_ptr<Resource> g_sampler = device->CreateSampler({
                                                                        SamplerFilter::kAnisotropic,
                                                                        SamplerTextureAddressMode::kWrap,
                                                                        SamplerComparisonFunc::kNever
                                                                });
    std::shared_ptr<Resource> diffuseTexture = CreateTexture(*device, *upload_command_list, ASSETS_PATH"model/export3dcoat/export3dcoat_lambert3SG_color.dds");


    // VRS resource
    std::vector<ShadingRate> shading_rate;
    uint32_t tile_size = device->GetShadingRateImageTileSize();
    uint32_t shading_rate_width = (rect.width + tile_size - 1) / tile_size;
    uint32_t shading_rate_height = (rect.height + tile_size - 1) / tile_size;
    for (uint32_t y = 0; y < shading_rate_height; ++y)
    {
        for (uint32_t x = 0; x < shading_rate_width; ++x)
        {
            if (x > (shading_rate_width / 2))
                shading_rate.emplace_back(ShadingRate::k4x4);
            else
                shading_rate.emplace_back(ShadingRate::k1x1);
        }
    }
    std::shared_ptr<Resource> shading_rate_texture = device->CreateTexture(BindFlag::kShadingRateSource | BindFlag::kCopyDest, gli::format::FORMAT_R8_UINT_PACK8, 1, shading_rate_width, shading_rate_height);
    size_t num_bytes = 0;
    size_t row_bytes = 0;
    GetFormatInfo(shading_rate_width, shading_rate_height, gli::format::FORMAT_R8_UINT_PACK8, num_bytes, row_bytes);
    upload_command_list->UpdateSubresource(shading_rate_texture, 0, shading_rate.data(), row_bytes, num_bytes);

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<ShaderPS, ShaderVS> program(*device);
    //program.ps.cbuffer.Settings.color = glm::vec4(1, 0, 0, 1);

    ViewDesc shading_rate_view_desc = {};
    shading_rate_view_desc.dimension = ResourceDimension::kTexture2D;
    shading_rate_view_desc.view_type = ViewType::kShadingRateSource;
    std::shared_ptr<View> shading_rate_view = device->CreateView(shading_rate_texture, shading_rate_view_desc);

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        RenderPassBeginDesc render_pass_desc = {};
        render_pass_desc.colors[0].texture = device->GetBackBuffer(i);
        render_pass_desc.colors[0].clear_color = { 0.0f, 0.2f, 0.4f, 1.0f };

        decltype(auto) command_list = device->CreateRenderCommandList();
        command_list->BeginEvent("VRS main pass");
        command_list->UseProgram(program);
        //command_list->Attach(program.ps.cbv.Settings, program.ps.cbuffer.Settings);
        command_list->SetViewport(0, 0, rect.width, rect.height);

        command_list->RSSetShadingRateImage(shading_rate_view);
        command_list->BeginRenderPass(render_pass_desc);

        command_list->IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
        command_list->IASetVertexBuffer(program.vs.ia.POSITION, pos);
        command_list->IASetVertexBuffer(program.vs.ia.TEXCOORD, uv);
        command_list->Attach(program.ps.srv.diffuseTexture, diffuseTexture);
        command_list->Attach(program.ps.sampler.g_sampler, g_sampler);
        command_list->DrawIndexed(6, 1, 0, 0, 0);

        command_list->EndRenderPass();
        command_list->EndEvent();
        command_list->Close();
        command_lists.emplace_back(command_list);
    }

    while (!app.PollEvents())
    {
        device->ExecuteCommandLists({ command_lists[device->GetFrameIndex()] });
        device->Present();
    }
    return 0;
}
