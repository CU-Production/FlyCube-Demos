#include "AppBox/AppBox.h"
#include "AppBox/ArgsParser.h"
#include "RenderDevice/RenderDevice.h"
#include "Texture/TextureLoader.h"

#include "ProgramRef/ShaderPS.h"
#include "ProgramRef/ShaderVS.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("16.3 - RaymarchingPrimitives", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();
    std::vector<uint32_t> ibuf = { 0, 1, 2, 0, 2, 3 };
    std::shared_ptr<Resource> index = device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * ibuf.size());
    upload_command_list->UpdateSubresource(index, 0, ibuf.data(), 0, 0);
    std::vector<glm::vec3> pbuf = {
            glm::vec3(-1.0,  1.0, 0.0),
            glm::vec3( 1.0,  1.0, 0.0),
            glm::vec3( 1.0, -1.0, 0.0),
            glm::vec3(-1.0, -1.0, 0.0),
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


    // ImGui
    auto render_target_view = device->GetBackBuffer(device->GetFrameIndex());

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<ShaderPS, ShaderVS> program(*device);
    //program.ps.cbuffer.Settings.color = glm::vec4(1, 0, 0, 1);

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        decltype(auto) command_list = device->CreateRenderCommandList();
        command_lists.emplace_back(command_list);
    }


    while (!app.PollEvents())
    {
        int32_t frameIndex = device->GetFrameIndex();
        device->Wait(command_lists[frameIndex]->GetFenceValue());
        command_lists[frameIndex]->Reset();
        {
            program.ps.cbuffer.constBuf.iResolution = { rect.width, rect.height};
            program.ps.cbuffer.constBuf.iTime = (float)glfwGetTime();
            double xpos, ypos;
            glfwGetCursorPos(app.GetWindow(), &xpos, &ypos);
            xpos = glm::clamp(xpos, 0.0, (double )rect.width);
            ypos = glm::clamp(ypos, 0.0, (double )rect.height);
            program.ps.cbuffer.constBuf.iMouse = {(float)xpos, (float)ypos};


            // Main Pass
            RenderPassBeginDesc render_pass_desc = {};
            render_pass_desc.colors[0].texture = device->GetBackBuffer(frameIndex);
            render_pass_desc.colors[0].clear_color = { 0.0f, 0.2f, 0.4f, 1.0f };

            command_lists[frameIndex]->BeginEvent("Main Pass");
            command_lists[frameIndex]->UseProgram(program);
            //command_list->Attach(program.ps.cbv.Settings, program.ps.cbuffer.Settings);
            command_lists[frameIndex]->SetViewport(0, 0, rect.width, rect.height);
            command_lists[frameIndex]->IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
            command_lists[frameIndex]->IASetVertexBuffer(program.vs.ia.POSITION, pos);
            command_lists[frameIndex]->IASetVertexBuffer(program.vs.ia.TEXCOORD, uv);
            command_lists[frameIndex]->Attach(program.ps.cbv.constBuf, program.ps.cbuffer.constBuf);
            command_lists[frameIndex]->BeginRenderPass(render_pass_desc);
            command_lists[frameIndex]->DrawIndexed(6, 1, 0, 0, 0);
            command_lists[frameIndex]->EndRenderPass();
            command_lists[frameIndex]->EndEvent();
        }

        command_lists[frameIndex]->Close();

        device->ExecuteCommandLists({ command_lists[device->GetFrameIndex()] });
        device->Present();
    }

    device->WaitForIdle();
    return 0;
}
