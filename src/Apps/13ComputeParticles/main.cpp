#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <RenderDevice/RenderDevice.h>
#include <Camera/Camera.h>

#include "ProgramRef/ShaderPS.h"
#include "ProgramRef/ShaderVS.h"
#include "ProgramRef/ParticleSpawn.h"
#include "ProgramRef/ParticleUpdate.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("13 - ComputeParticles", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    app.SetGpuName(device->GetGpuName());

    Camera camera;
    camera.SetCameraPos(glm::vec3(0.0, 2.0, 10.0));
//    camera.SetCameraYaw(-1.75f);
//    camera.SetCameraPitch(0);
    camera.SetCameraPitch(0);
    //camera.SetCameraYaw(-178.0f);
    //camera.SetCameraYaw(-1.75f);
    camera.SetCameraYaw(-90.0f);
    camera.SetViewport(rect.width, rect.height);

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();
    std::vector<uint32_t> ibuf = { 0, 1, 2, 0, 2, 3 };
    std::shared_ptr<Resource> index = device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * ibuf.size());
    upload_command_list->UpdateSubresource(index, 0, ibuf.data(), 0, 0);
    std::vector<glm::vec3> pbuf = {
            glm::vec3(-0.25, -0.25, 0.0),
            glm::vec3(-0.25,  0.25, 0.0),
            glm::vec3( 0.25,  0.25, 0.0),
            glm::vec3( 0.25, -0.25, 0.0),
    };
    std::vector<glm::vec3> colorbuf = {
            glm::vec3(1.0, 0.0, 0.0),
            glm::vec3(0.0, 1.0, 0.0),
            glm::vec3(0.0, 0.0, 1.0),
            glm::vec3(1.0, 1.0, 1.0),
    };
    std::shared_ptr<Resource> pos = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec3) * pbuf.size());
    upload_command_list->UpdateSubresource(pos, 0, pbuf.data(), 0, 0);
    std::shared_ptr<Resource> col = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec3) * colorbuf.size());
    upload_command_list->UpdateSubresource(col, 0, colorbuf.data(), 0, 0);

    std::shared_ptr<Resource> dsv = device->CreateTexture(BindFlag::kDepthStencil, gli::format::FORMAT_D32_SFLOAT_PACK32, 1, rect.width, rect.height, 1);
    std::shared_ptr<Resource> sampler = device->CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever
    });

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<ShaderPS, ShaderVS> program(*device);

    program.vs.cbuffer.ConstantBuf.model = glm::transpose(glm::mat4(1.0));
    program.vs.cbuffer.ConstantBuf.view = glm::transpose(camera.GetViewMatrix());
    program.vs.cbuffer.ConstantBuf.projection = glm::transpose(camera.GetProjectionMatrix());
    program.vs.cbuffer.ConstantBuf.mvp = glm::transpose(glm::mat4(1.0)) * glm::transpose(camera.GetViewMatrix()) *glm::transpose(camera.GetProjectionMatrix());

    // Global Particle System info
    const int NUM_PARTICLES = 2048;

    struct EmitterInfo{
        glm::vec3 emitterPos;
        glm::vec2 particleLifeRange;
        glm::vec3 minVelocity;
        glm::vec3 maxVelocity;
        glm::vec3 gravity;
    };
    EmitterInfo emitterInfo = {};
    emitterInfo.emitterPos = glm::vec3(0.0, 2.0, 0.0);
    emitterInfo.particleLifeRange = glm::vec2(1, 5.0);
    emitterInfo.minVelocity = glm::vec3(-2.5, -2.5, -2.5);
    emitterInfo.maxVelocity = glm::vec3(2.5, 2.5, 2.5);
    emitterInfo.gravity = glm::vec3(0.0, -9.8, 0.0);

    struct Particle
    {
        glm::vec3 position;
        float lifetime;
        glm::vec3 scale;
        glm::vec3 velocity;
    };
    std::shared_ptr<Resource> particle_spawn_buffer = device->CreateBuffer(BindFlag::kUnorderedAccess |BindFlag::kIndirectBuffer | BindFlag::kCopyDest, NUM_PARTICLES * sizeof(Particle));
    std::shared_ptr<Resource> particle_update_buffer = device->CreateBuffer(BindFlag::kUnorderedAccess |BindFlag::kIndirectBuffer | BindFlag::kCopyDest, NUM_PARTICLES * sizeof(Particle));

    // particle spawn pass
    {
        ProgramHolder<ParticleSpawn> particleSpawnProgram(*device);

        particleSpawnProgram.cs.cbuffer.global.time = (float)glfwGetTime();
        particleSpawnProgram.cs.cbuffer.emitterInfo.emitterPos = emitterInfo.emitterPos;
        particleSpawnProgram.cs.cbuffer.emitterInfo.particleLifeRange = emitterInfo.particleLifeRange;
        particleSpawnProgram.cs.cbuffer.emitterInfo.minVelocity = emitterInfo.minVelocity;
        particleSpawnProgram.cs.cbuffer.emitterInfo.maxVelocity = emitterInfo.maxVelocity;
        particleSpawnProgram.cs.cbuffer.emitterInfo.gravity = emitterInfo.gravity;

        std::shared_ptr<RenderCommandList> particle_spawn_command_list = device->CreateRenderCommandList();

        particle_spawn_command_list->BeginEvent("Particle Spawn Pass");
        particle_spawn_command_list->UseProgram(particleSpawnProgram);
        particle_spawn_command_list->Attach(particleSpawnProgram.cs.uav.SpawnParticles, particle_spawn_buffer);
        particle_spawn_command_list->Attach(particleSpawnProgram.cs.cbv.global, particleSpawnProgram.cs.cbuffer.global);
        particle_spawn_command_list->Attach(particleSpawnProgram.cs.cbv.emitterInfo, particleSpawnProgram.cs.cbuffer.emitterInfo);
        particle_spawn_command_list->Dispatch(NUM_PARTICLES / 32, 1, 1);
        particle_spawn_command_list->EndEvent();

        particle_spawn_command_list->Close();
        device->ExecuteCommandLists({ particle_spawn_command_list });
    }

    ProgramHolder<ParticleUpdate> particleUpdateProgram(*device);
    particleUpdateProgram.cs.cbuffer.emitterInfo.emitterPos = emitterInfo.emitterPos;
    particleUpdateProgram.cs.cbuffer.emitterInfo.particleLifeRange = emitterInfo.particleLifeRange;
    particleUpdateProgram.cs.cbuffer.emitterInfo.minVelocity = emitterInfo.minVelocity;
    particleUpdateProgram.cs.cbuffer.emitterInfo.maxVelocity = emitterInfo.maxVelocity;
    particleUpdateProgram.cs.cbuffer.emitterInfo.gravity = emitterInfo.gravity;

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        decltype(auto) command_list = device->CreateRenderCommandList();
        command_list->Close();
        command_lists.emplace_back(command_list);
    }

    float preGlobalTime = (float)glfwGetTime();

    while (!app.PollEvents())
    {
        int32_t frameIndex = device->GetFrameIndex();
        device->Wait(command_lists[frameIndex]->GetFenceValue());
        command_lists[frameIndex]->Reset();
        RenderPassBeginDesc render_pass_desc = {};
        {
            render_pass_desc.colors[0].texture = device->GetBackBuffer(frameIndex);
            render_pass_desc.colors[0].clear_color = { 0.0f, 0.2f, 0.4f, 1.0f };
            render_pass_desc.depth_stencil.texture = dsv;
            render_pass_desc.depth_stencil.clear_depth = 1.0f;
        }
        {
            command_lists[frameIndex]->BeginEvent("Particle Pass");

            particleUpdateProgram.cs.cbuffer.global.time = (float)glfwGetTime();
            particleUpdateProgram.cs.cbuffer.global.deltaTime = particleUpdateProgram.cs.cbuffer.global.time - preGlobalTime;
            preGlobalTime = particleUpdateProgram.cs.cbuffer.global.time;
            command_lists[frameIndex]->UseProgram(particleUpdateProgram);
            command_lists[frameIndex]->Attach(particleUpdateProgram.cs.srv.SpawnParticles, particle_spawn_buffer);
            command_lists[frameIndex]->Attach(particleUpdateProgram.cs.uav.UpdateParticles, particle_update_buffer);
            command_lists[frameIndex]->Attach(particleUpdateProgram.cs.cbv.emitterInfo, particleUpdateProgram.cs.cbuffer.emitterInfo);
            command_lists[frameIndex]->Attach(particleUpdateProgram.cs.cbv.global, particleUpdateProgram.cs.cbuffer.global);
            command_lists[frameIndex]->Dispatch(NUM_PARTICLES / 32, 1, 1);

            command_lists[frameIndex]->EndEvent();
        }
        {
            command_lists[frameIndex]->BeginEvent("DrawIndirect Pass");
            command_lists[frameIndex]->UseProgram(program);
            command_lists[frameIndex]->SetViewport(0, 0, rect.width, rect.height);
            command_lists[frameIndex]->IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
            command_lists[frameIndex]->IASetVertexBuffer(program.vs.ia.POSITION, pos);
            command_lists[frameIndex]->IASetVertexBuffer(program.vs.ia.COLOR, col);
            command_lists[frameIndex]->Attach(program.vs.cbv.ConstantBuf, program.vs.cbuffer.ConstantBuf);
            command_lists[frameIndex]->Attach(program.vs.srv.SpawnParticles, particle_update_buffer);
            command_lists[frameIndex]->BeginRenderPass(render_pass_desc);
            command_lists[frameIndex]->DrawIndexed(6, NUM_PARTICLES, 0, 0, 0);
            command_lists[frameIndex]->EndRenderPass();
            command_lists[frameIndex]->EndEvent();
        }

        device->ExecuteCommandLists({ command_lists[frameIndex] });
        device->Present();
    }
    device->WaitForIdle();
    return 0;
}
