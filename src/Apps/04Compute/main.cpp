#include "AppBox/AppBox.h"
#include "AppBox/ArgsParser.h"
#include "RenderDevice/RenderDevice.h"
#include "Texture/TextureLoader.h"

#include "ProgramRef/RayTracing.h"
#include "ProgramRef/Noise.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("04 - Compute", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();

    const static int uavWidth = 512;
    const static int uavHeight = 512;
    std::shared_ptr<Resource> uav = device->CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kShaderResource | BindFlag::kCopySource,
                                                          device->GetFormat(), 1, uavWidth, uavHeight);
    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<Noise> program(*device);
    program.cs.cbuffer.computeUniformBlock.time = (float)glfwGetTime();

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        decltype(auto) command_list = device->CreateRenderCommandList();
        command_lists.emplace_back(command_list);
    }

    while (!app.PollEvents())
    {
        program.cs.cbuffer.computeUniformBlock.time = (float)glfwGetTime();

        int32_t frameIndex = device->GetFrameIndex();
        command_lists[frameIndex]->Reset();
        command_lists[frameIndex]->BeginEvent("Compute Pass");
        command_lists[frameIndex]->UseProgram(program);
        command_lists[frameIndex]->Attach(program.cs.cbv.computeUniformBlock, program.cs.cbuffer.computeUniformBlock);
        command_lists[frameIndex]->Attach(program.cs.uav.Tex0, uav);
        command_lists[frameIndex]->Dispatch(uavWidth/8, uavHeight/8, 1);
        command_lists[frameIndex]->CopyTexture(uav, device->GetBackBuffer(frameIndex), { { uavWidth, uavHeight, 1 } });
        command_lists[frameIndex]->EndEvent();
        command_lists[frameIndex]->Close();

        device->ExecuteCommandLists({ command_lists[frameIndex] });
        device->Present();
    }
    return 0;
}
