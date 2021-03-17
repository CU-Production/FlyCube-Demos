#include "AppBox/AppBox.h"
#include "AppBox/ArgsParser.h"
#include "RenderDevice/RenderDevice.h"
#include "Texture/TextureLoader.h"
#include "ProgramRef/ShaderPS.h"
#include "ProgramRef/ShaderVS.h"
#include "ProgramRef/PostProcessingPS.h"
#include "ProgramRef/PostProcessingVS.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("03 - MultiPass", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();
    std::vector<uint32_t> ibuf = { 0, 1, 2, 0, 2, 3 };
    std::shared_ptr<Resource> index = device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * ibuf.size());
    upload_command_list->UpdateSubresource(index, 0, ibuf.data(), 0, 0);

    // Main
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

    // PP
    std::vector<glm::vec3> ppPosBuf = {
            glm::vec3(-1.0,  1.0, 0.0),
            glm::vec3( 1.0,  1.0, 0.0),
            glm::vec3( 1.0, -1.0, 0.0),
            glm::vec3(-1.0, -1.0, 0.0),
    };
    std::vector<glm::vec2> ppUVBuf = {
            glm::vec2(0.0, 0.0),
            glm::vec2(1.0, 0.0),
            glm::vec2(1.0, 1.0),
            glm::vec2(0.0, 1.0),
    };
    std::shared_ptr<Resource> ppPos = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec3) * ppPosBuf.size());
    upload_command_list->UpdateSubresource(ppPos, 0, ppPosBuf.data(), 0, 0);
    std::shared_ptr<Resource> ppUV = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec2) * ppUVBuf.size());
    upload_command_list->UpdateSubresource(ppUV, 0, ppUVBuf.data(), 0, 0);

    std::shared_ptr<Resource> g_sampler = device->CreateSampler({
                                                                        SamplerFilter::kAnisotropic,
                                                                        SamplerTextureAddressMode::kWrap,
                                                                        SamplerComparisonFunc::kNever
                                                                });
    std::shared_ptr<Resource> diffuseTexture = CreateTexture(*device, *upload_command_list, ASSETS_PATH"model/export3dcoat/export3dcoat_lambert3SG_color.dds");

    // create RT
    const static int rtWidth = 1024;
    const static int rtHeight = 1024;
    std::shared_ptr<Resource> renderTexture = device->CreateTexture(BindFlag::kRenderTarget | BindFlag::kShaderResource, gli::format::FORMAT_RGBA16_SFLOAT_PACK16, 1,
                                                                    rtWidth, rtHeight, 1);

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<ShaderPS, ShaderVS> mainProgram(*device);
    ProgramHolder<PostProcessingPS, PostProcessingVS> ppProgram(*device);
    //mainProgram.ps.cbuffer.Settings.color = glm::vec4(1, 0, 0, 1);

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        decltype(auto) command_list = device->CreateRenderCommandList();
        {
            // Main Pass
            RenderPassBeginDesc main_pass_desc = {};
            main_pass_desc.colors[0].texture = renderTexture;
            main_pass_desc.colors[0].clear_color = {0.0f, 0.2f, 0.4f, 1.0f };


            command_list->BeginEvent("Main Pass");

            command_list->UseProgram(mainProgram);
            //command_list->Attach(mainProgram.ps.cbv.Settings, mainProgram.ps.cbuffer.Settings);
            command_list->SetViewport(0, 0, rtWidth, rtHeight);
            command_list->IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
            command_list->IASetVertexBuffer(mainProgram.vs.ia.POSITION, pos);
            command_list->IASetVertexBuffer(mainProgram.vs.ia.TEXCOORD, uv);
            command_list->Attach(mainProgram.ps.srv.diffuseTexture, diffuseTexture);
            command_list->Attach(mainProgram.ps.sampler.g_sampler, g_sampler);

            command_list->BeginRenderPass(main_pass_desc);
            command_list->DrawIndexed(6, 1, 0, 0, 0);
            command_list->EndRenderPass();

            command_list->EndEvent();
        }
        {
            // PostProcessing Pass
            RenderPassBeginDesc postprocssing_pass_desc = {};
            postprocssing_pass_desc.colors[0].texture = device->GetBackBuffer(i);
            postprocssing_pass_desc.colors[0].clear_color = {0.0f, 0.2f, 0.4f, 1.0f };


            command_list->BeginEvent("PostProcessing Pass");

            command_list->UseProgram(ppProgram);
            //command_list->Attach(mainProgram.ps.cbv.Settings, mainProgram.ps.cbuffer.Settings);
            command_list->SetViewport(0, 0, rect.width, rect.height);
            command_list->IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
            command_list->IASetVertexBuffer(ppProgram.vs.ia.POSITION, ppPos);
            command_list->IASetVertexBuffer(ppProgram.vs.ia.TEXCOORD, ppUV);
            command_list->Attach(ppProgram.ps.srv.screenTexture, renderTexture);
            command_list->Attach(ppProgram.ps.sampler.g_sampler, g_sampler);

            command_list->BeginRenderPass(postprocssing_pass_desc);
            command_list->DrawIndexed(6, 1, 0, 0, 0);
            command_list->EndRenderPass();

            command_list->EndEvent();
        }
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
