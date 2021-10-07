#include "AppBox/AppBox.h"
#include "AppBox/ArgsParser.h"
#include "RenderDevice/RenderDevice.h"
#include "Texture/TextureLoader.h"

#include "ProgramRef/ShaderCS.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("16.4 - CS Raymarching", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();

    // UAV
    const static int uavWidth = rect.width;
    const static int uavHeight = rect.height;
    std::shared_ptr<Resource> uav = device->CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kShaderResource | BindFlag::kCopySource,
                                                          device->GetFormat(), 1, uavWidth, uavHeight);


    // ImGui
    auto render_target_view = device->GetBackBuffer(device->GetFrameIndex());

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<ShaderCS> program(*device);
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
            program.cs.cbuffer.constBuf.iResolution = { rect.width, rect.height};
            program.cs.cbuffer.constBuf.iTime = (float)glfwGetTime();
            double xpos, ypos;
            glfwGetCursorPos(app.GetWindow(), &xpos, &ypos);
            xpos = glm::clamp(xpos, 0.0, (double )rect.width);
            ypos = glm::clamp(ypos, 0.0, (double )rect.height);
            program.cs.cbuffer.constBuf.iMouse = {(float)xpos, (float)ypos};


            // Main Pass
            RenderPassBeginDesc render_pass_desc = {};
            render_pass_desc.colors[0].texture = device->GetBackBuffer(frameIndex);
            render_pass_desc.colors[0].clear_color = { 0.0f, 0.2f, 0.4f, 1.0f };

            command_lists[frameIndex]->BeginEvent("Main CS Pass");
            command_lists[frameIndex]->UseProgram(program);
            //command_list->Attach(program.ps.cbv.Settings, program.ps.cbuffer.Settings);
            command_lists[frameIndex]->SetViewport(0, 0, rect.width, rect.height);
            command_lists[frameIndex]->Attach(program.cs.uav.imageData, uav);
            command_lists[frameIndex]->Attach(program.cs.cbv.constBuf, program.cs.cbuffer.constBuf);
            command_lists[frameIndex]->Dispatch((uint32_t(uavWidth) + 8 - 1)/8, (uint32_t(uavHeight) + 8 - 1)/8, 1);
            command_lists[frameIndex]->CopyTexture(uav, device->GetBackBuffer(frameIndex), { { (uint32_t)uavWidth, (uint32_t)uavHeight, 1 } });
            command_lists[frameIndex]->EndEvent();
        }

        command_lists[frameIndex]->Close();

        device->ExecuteCommandLists({ command_lists[device->GetFrameIndex()] });
        device->Present();
    }

    device->WaitForIdle();
    return 0;
}
