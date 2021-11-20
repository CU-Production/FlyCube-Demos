#include "AppBox/AppBox.h"
#include "AppBox/ArgsParser.h"
#include "RenderDevice/RenderDevice.h"
#include "Geometry/Geometry.h"
#include "Camera/Camera.h"
#include <glm/gtx/transform.hpp>

#include "ProgramRef/Shader_PS.h"
#include "ProgramRef/Shader_VS.h"
#include "ProgramRef/Skinning_CS.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("19 - CS Skinning", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();
    auto dsv = device->CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, rect.width, rect.height, 1);
    auto sampler = device->CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever
    });

    Camera camera;
    camera.SetCameraPos(glm::vec3(0.0, 0.0, 1.0));
    camera.SetViewport(rect.width, rect.height);
    camera.movement_speed_ = 1.0f;
    camera.z_near_ = 0.001f;

    Model model(*device, *upload_command_list, ASSETS_PATH"model/Mannequin_Animation/source/Mannequin_Animation.FBX");
//    model.matrix = glm::scale(glm::vec3(0.01f)) * glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model.matrix = glm::scale(glm::vec3(0.01f));

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<Shader_VS, Shader_PS> program(*device);
    ProgramHolder<Skinning_CS> skinningProgram(*device);

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

        program.vs.cbuffer.Constants.World = glm::transpose(model.matrix);
        program.vs.cbuffer.Constants.WorldView = program.vs.cbuffer.Constants.World * glm::transpose(camera.GetViewMatrix());
        program.vs.cbuffer.Constants.WorldViewProj = program.vs.cbuffer.Constants.WorldView * glm::transpose(camera.GetProjectionMatrix());

        RenderPassBeginDesc render_pass_desc = {};
        render_pass_desc.colors[0].texture = device->GetBackBuffer(frameIndex);
        render_pass_desc.colors[0].clear_color = { 0.0f, 0.2f, 0.4f, 1.0f };

        {
            model.bones.UpdateAnimation(*device, *command_lists[frameIndex], glfwGetTime());

            command_lists[frameIndex]->BeginEvent("CS Skinning Pass");
            command_lists[frameIndex]->UseProgram(skinningProgram);

            command_lists[frameIndex]->Attach(skinningProgram.cs.srv.index_buffer, model.ia.indices.GetBuffer());

            command_lists[frameIndex]->Attach(skinningProgram.cs.srv.bone_info, model.bones.GetBonesInfo());
            command_lists[frameIndex]->Attach(skinningProgram.cs.srv.gBones, model.bones.GetBone());

            command_lists[frameIndex]->Attach(skinningProgram.cs.srv.bones_offset, model.ia.bones_offset.GetBuffer());
            command_lists[frameIndex]->Attach(skinningProgram.cs.srv.bones_count, model.ia.bones_count.GetBuffer());

            command_lists[frameIndex]->Attach(skinningProgram.cs.srv.in_position, model.ia.positions.GetBuffer());
            command_lists[frameIndex]->Attach(skinningProgram.cs.srv.in_normal, model.ia.normals.GetBuffer());
            command_lists[frameIndex]->Attach(skinningProgram.cs.srv.in_tangent, model.ia.tangents.GetBuffer());

            command_lists[frameIndex]->Attach(skinningProgram.cs.uav.out_position, model.ia.positions.GetDynamicBuffer());
            command_lists[frameIndex]->Attach(skinningProgram.cs.uav.out_normal, model.ia.normals.GetDynamicBuffer());
            command_lists[frameIndex]->Attach(skinningProgram.cs.uav.out_tangent, model.ia.tangents.GetDynamicBuffer());

            for (auto& range : model.ia.ranges)
            {
                skinningProgram.cs.cbuffer.cb.IndexCount = range.index_count;
                skinningProgram.cs.cbuffer.cb.StartIndexLocation = range.start_index_location;
                skinningProgram.cs.cbuffer.cb.BaseVertexLocation = range.base_vertex_location;
                command_lists[frameIndex]->Attach(skinningProgram.cs.cbv.cb, skinningProgram.cs.cbuffer.cb);
                command_lists[frameIndex]->Dispatch((range.index_count + 64 - 1) / 64, 1, 1);
            }
            command_lists[frameIndex]->EndEvent();
        }
        {
            command_lists[frameIndex]->BeginEvent("Mesh Pass");
            command_lists[frameIndex]->UseProgram(program);
            command_lists[frameIndex]->SetViewport(0, 0, rect.width, rect.height);
            command_lists[frameIndex]->BeginRenderPass(render_pass_desc);

            model.ia.indices.Bind(*command_lists[frameIndex]);
            model.ia.positions.BindToSlot(*command_lists[frameIndex], program.vs.ia.POSITION);
            model.ia.texcoords.BindToSlot(*command_lists[frameIndex], program.vs.ia.TEXCOORD);
            for (auto& range : model.ia.ranges)
            {
                command_lists[frameIndex]->Attach(program.vs.cbv.Constants, program.vs.cbuffer.Constants);
                command_lists[frameIndex]->Attach(program.ps.sampler.g_sampler, sampler);
                command_lists[frameIndex]->Attach(program.ps.srv.diffuseTexture, model.GetMaterial(range.id).texture.albedo);
                command_lists[frameIndex]->DrawIndexed(range.index_count, 1, range.start_index_location, range.base_vertex_location, 0);
            }
            command_lists[frameIndex]->EndRenderPass();
            command_lists[frameIndex]->EndEvent();
        }
        command_lists[frameIndex]->Close();

        device->ExecuteCommandLists({ command_lists[frameIndex] });
        device->Present();
    }
    return 0;
}
