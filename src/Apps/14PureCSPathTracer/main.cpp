#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Geometry/Geometry.h>
#include <Camera/Camera.h>
#include <Utilities/FormatHelper.h>
#include <glm/gtx/transform.hpp>
#include <stdexcept>

#include "ProgramRef/PathTracer.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("14 - PureCSPathTracer", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();

    // UAV
    const static int uavWidth = rect.width;
    const static int uavHeight = rect.height;
    std::shared_ptr<Resource> uav = device->CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kShaderResource | BindFlag::kCopySource,
                                                          device->GetFormat(), 1, uavWidth, uavHeight);

    struct Material
    {
        glm::vec3 emmitance;
        glm::vec3 reflectance;
        float roughness;
        float opacity;
    };

    struct Box
    {
        Material material;
        glm::vec3 halfSize;
        glm::mat3 rotation;
        glm::vec3 position;
    };

    struct Sphere
    {
        Material material;
        glm::vec3 position;
        float radius;
    };

    const int SPHERE_COUNT = 3;
    const int BOX_COUNT = 8;
    std::shared_ptr<Resource> sphere_uav_buffer = device->CreateBuffer(BindFlag::kUnorderedAccess |BindFlag::kIndirectBuffer | BindFlag::kCopyDest, SPHERE_COUNT * sizeof(Sphere));
    std::shared_ptr<Resource> box_uav_buffer = device->CreateBuffer(BindFlag::kUnorderedAccess |BindFlag::kIndirectBuffer | BindFlag::kCopyDest, BOX_COUNT * sizeof(Box));

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<PathTracer> program(*device);
    program.cs.cbuffer.resolution.screenWidth = uavWidth;
    program.cs.cbuffer.resolution.screenHeight = uavHeight;

    program.cs.cbuffer.uniformBlock.uPosition = glm::vec3(0.0, 0.0, 30.0);
    program.cs.cbuffer.uniformBlock.uTime = (float)glfwGetTime();
    program.cs.cbuffer.uniformBlock.uSamples = 64;

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        decltype(auto) command_list = device->CreateRenderCommandList();
        command_list->BeginEvent("PathTracer Pass");
        command_list->UseProgram(program);
        command_list->Attach(program.cs.cbv.resolution, program.cs.cbuffer.resolution);
        command_list->Attach(program.cs.cbv.uniformBlock, program.cs.cbuffer.uniformBlock);
        command_list->Attach(program.cs.uav.imageData, uav);
        command_list->Attach(program.cs.uav.boxes, box_uav_buffer);
        command_list->Attach(program.cs.uav.spheres, sphere_uav_buffer);
        command_list->Dispatch((uint32_t(uavWidth) + 8 - 1)/8, (uint32_t(uavHeight) + 8 - 1)/8, 1);
        command_list->CopyTexture(uav, device->GetBackBuffer(i), { { (uint32_t)uavWidth, (uint32_t)uavHeight, 1 } });
        command_list->EndEvent();
        command_list->Close();
        command_lists.emplace_back(command_list);
    }

    while (!app.PollEvents())
    {
        device->ExecuteCommandLists({ command_lists[device->GetFrameIndex()] });
        device->Present();
    }
    device->WaitForIdle();
    return 0;
}
