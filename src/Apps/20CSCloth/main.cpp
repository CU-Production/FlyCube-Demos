#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Geometry/Geometry.h>
#include <Camera/Camera.h>
#include <Utilities/FormatHelper.h>
#include <memory>
#include <glm/gtx/transform.hpp>
#include <stdexcept>
#include <SOIL.h>

#include "ProgramRef/Shader_PS.h"
#include "ProgramRef/Shader_VS.h"
#include "ProgramRef/ClothSim_CS.h"

// ImGui
#include "../08ImGui/ImGuiPass.h"

std::map<int, bool> keys;

void UpdateKeyStateInternal(GLFWwindow* window, int KeyCode)
{
    if (glfwGetKey(window, KeyCode) == GLFW_PRESS)
        keys[KeyCode] = true;
    if (glfwGetKey(window, KeyCode) == GLFW_RELEASE)
        keys[KeyCode] = false;
}

void UpdateKeyState(GLFWwindow* window)
{
    UpdateKeyStateInternal(window, GLFW_KEY_W);
    UpdateKeyStateInternal(window, GLFW_KEY_S);
    UpdateKeyStateInternal(window, GLFW_KEY_A);
    UpdateKeyStateInternal(window, GLFW_KEY_D);
    UpdateKeyStateInternal(window, GLFW_KEY_Q);
    UpdateKeyStateInternal(window, GLFW_KEY_E);
}

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("20 - GPU Cloth Simulation", settings);
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

    std::shared_ptr<Resource> diffuseTexture = CreateTexture(*device, *upload_command_list, ASSETS_PATH"textures/me_textile.png");

    Camera camera;
    camera.SetCameraPos(glm::vec3(0.0, 0.0, 1.0));
    camera.SetViewport(rect.width, rect.height);
    camera.movement_speed_ = 1.0f;
    camera.z_near_ = 0.001f;

    // Cloth ping-pang buffer
    std::array<std::unique_ptr<IAVertexBuffer>, 2> cloth_vertex_buffer;
    std::array<std::unique_ptr<IAVertexBuffer>, 2> cloth_velocity_buffer;
    std::unique_ptr<IAVertexBuffer> cloth_texcoord_buffer;
    std::unique_ptr<IAIndexBuffer> cloth_index_buffer;

    glm::vec2 clothSize = {4.0f, 3.0f};
    int ParticleCountX = 40;
    int ParticleCountY = 40;
    float dx = clothSize.x / (ParticleCountX - 1);
    float dy = clothSize.y / (ParticleCountY - 1);
    float ds = 1.0f / (ParticleCountX - 1);
    float dt = 1.0f / (ParticleCountY - 1);
    float RestLengthHoriz = dx;
    float RestLengthVert = dy;
    float RestLengthDiag = sqrtf(dx * dx + dy * dy);

    static int clothIdx = 0;
    const static int particleCount = ParticleCountX * ParticleCountY;
    static bool firstSimulateFrame = true;

    std::vector<glm::vec3> initPos;
    std::vector<glm::vec2> initTc;

    glm::vec3 p(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < ParticleCountY; ++i) {
        for (int j = 0; j < ParticleCountX; ++j) {
            p.x = dx * j;
            p.y = dy * i;
            p.z = 0.0f;
            initPos.push_back(p);
            initTc.emplace_back(ds * j, dt * i);
        }
    }
    cloth_vertex_buffer[0] = std::make_unique<IAVertexBuffer>(*device, *upload_command_list, initPos);
    cloth_vertex_buffer[1] = std::make_unique<IAVertexBuffer>(*device, *upload_command_list, initPos);
    cloth_texcoord_buffer = std::make_unique<IAVertexBuffer>(*device, *upload_command_list, initTc);

    std::vector<glm::vec3> velocity;
    for (int i = 0; i < particleCount; ++i) {
        velocity.emplace_back(0.0, 0.0, 0.0);
    }
    cloth_velocity_buffer[0] = std::make_unique<IAVertexBuffer>(*device, *upload_command_list, velocity);
    cloth_velocity_buffer[1] = std::make_unique<IAVertexBuffer>(*device, *upload_command_list, velocity);

    std::vector<uint32_t> iBuf;
    for (int i = 0; i < ParticleCountY - 1; ++i) {
        for (int j = 0; j < ParticleCountX - 1; ++j) {
            int idx00 = i * ParticleCountY + j;
            int idx10 = i * ParticleCountY + j + 1;
            int idx01 = (i + 1) * ParticleCountY + j;
            int idx11 =  (i + 1) * ParticleCountY + j + 1;

            iBuf.push_back(idx00);
            iBuf.push_back(idx11);
            iBuf.push_back(idx10);

            iBuf.push_back(idx00);
            iBuf.push_back(idx01);
            iBuf.push_back(idx11);

        }
    }
    cloth_index_buffer = std::make_unique<IAIndexBuffer>(*device, *upload_command_list, iBuf, gli::format::FORMAT_R32_UINT_PACK32);

    // ImGui
    auto render_target_view = device->GetBackBuffer(device->GetFrameIndex());
    ImGuiPass imgui_pass(*device, *upload_command_list, { render_target_view }, rect.width, rect.height, app.GetWindow());

    app.SubscribeEvents(&imgui_pass, &imgui_pass);

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<Shader_PS, Shader_VS> program(*device);
    ProgramHolder<ClothSim_CS> clothSimProgram(*device);

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        decltype(auto) command_list = device->CreateRenderCommandList();
        command_lists.emplace_back(command_list);
    }

    float delta_time;
    float last_frame_time = static_cast<float>(glfwGetTime());;
    double last_x_pos, last_y_pos;
    glfwGetCursorPos(app.GetWindow(), &last_x_pos, &last_y_pos);

    float alpha = 0.029f;
    float stepSize = 0.0013;
    int maxStepCount = 128;
    bool bUsingZFilter = true;

    while (!app.PollEvents())
    {
        // Camera movement
        {
            float currentFrame = static_cast<float>(glfwGetTime());
            delta_time = currentFrame - last_frame_time;
            last_frame_time = currentFrame;

            // Mouse
            {
                double xpos, ypos;
                glfwGetCursorPos(app.GetWindow(), &xpos, &ypos);

                double xoffset = xpos - last_x_pos;
                double yoffset = last_y_pos - ypos;

                last_x_pos = xpos;
                last_y_pos = ypos;

                if (glfwGetInputMode(app.GetWindow(), GLFW_CURSOR) != GLFW_CURSOR_NORMAL)
                    camera.ProcessMouseMovement((float)xoffset, (float)yoffset);
            }


            // Keyboard
            {
                UpdateKeyState(app.GetWindow());

                if (keys[GLFW_KEY_W])
                    camera.ProcessKeyboard(CameraMovement::kForward, delta_time);
                if (keys[GLFW_KEY_S])
                    camera.ProcessKeyboard(CameraMovement::kBackward, delta_time);
                if (keys[GLFW_KEY_A])
                    camera.ProcessKeyboard(CameraMovement::kLeft, delta_time);
                if (keys[GLFW_KEY_D])
                    camera.ProcessKeyboard(CameraMovement::kRight, delta_time);
                if (keys[GLFW_KEY_Q])
                    camera.ProcessKeyboard(CameraMovement::kDown, delta_time);
                if (keys[GLFW_KEY_E])
                    camera.ProcessKeyboard(CameraMovement::kUp, delta_time);
                if (keys[GLFW_KEY_ESCAPE])
                    glfwSetWindowShouldClose(app.GetWindow(), GLFW_TRUE);
            }
        }

        int32_t frameIndex = device->GetFrameIndex();
        device->Wait(command_lists[frameIndex]->GetFenceValue());
        command_lists[frameIndex]->Reset();
        {
            // CS Simulation pass
            clothIdx = 1 - clothIdx;

            clothSimProgram.cs.cbuffer.constants.gravity = glm::vec3(0.0f, -9.8f, 0.0f);
//            clothSimProgram.cs.cbuffer.constants.gravity = glm::vec3(0.0f, -0.98f, 0.0f);
            clothSimProgram.cs.cbuffer.constants.RestLengthHoriz = RestLengthHoriz;
            clothSimProgram.cs.cbuffer.constants.RestLengthVert = RestLengthVert;
            clothSimProgram.cs.cbuffer.constants.RestLengthDiag = RestLengthDiag;

            command_lists[frameIndex]->BeginEvent("GPU Cloth Simulation Pass");
            command_lists[frameIndex]->UseProgram(clothSimProgram);
            command_lists[frameIndex]->Attach(clothSimProgram.cs.cbv.constants, clothSimProgram.cs.cbuffer.constants);

            if (firstSimulateFrame)
            {
                command_lists[frameIndex]->Attach(clothSimProgram.cs.srv.in_position, cloth_vertex_buffer[clothIdx]->GetBuffer());
                command_lists[frameIndex]->Attach(clothSimProgram.cs.srv.in_velocity, cloth_velocity_buffer[clothIdx]->GetBuffer());
                command_lists[frameIndex]->Attach(clothSimProgram.cs.uav.out_position, cloth_vertex_buffer[1-clothIdx]->GetDynamicBuffer());
                command_lists[frameIndex]->Attach(clothSimProgram.cs.uav.out_velocity, cloth_velocity_buffer[1-clothIdx]->GetDynamicBuffer());
                firstSimulateFrame = false;
            }
            else
            {
                command_lists[frameIndex]->Attach(clothSimProgram.cs.srv.in_position, cloth_vertex_buffer[clothIdx]->GetDynamicBuffer());
                command_lists[frameIndex]->Attach(clothSimProgram.cs.srv.in_velocity, cloth_velocity_buffer[clothIdx]->GetDynamicBuffer());
                command_lists[frameIndex]->Attach(clothSimProgram.cs.uav.out_position, cloth_vertex_buffer[1-clothIdx]->GetDynamicBuffer());
                command_lists[frameIndex]->Attach(clothSimProgram.cs.uav.out_velocity, cloth_velocity_buffer[1-clothIdx]->GetDynamicBuffer());
            }

            command_lists[frameIndex]->Dispatch((ParticleCountX + 8 - 1) / 8, (ParticleCountY + 8 - 1) / 8, 1);
            command_lists[frameIndex]->EndEvent();
        }
        {
            // Draw pass
            program.vs.cbuffer.Constants.World = glm::transpose(glm::scale(glm::vec3(0.5f)) * glm::translate(glm::vec3(-1.0f, -1.0f, 0.0f)));
            program.vs.cbuffer.Constants.WorldView = program.vs.cbuffer.Constants.World * glm::transpose(camera.GetViewMatrix());
            program.vs.cbuffer.Constants.WorldViewProj = program.vs.cbuffer.Constants.WorldView * glm::transpose(camera.GetProjectionMatrix());
            program.vs.cbuffer.Constants.CameraPos = camera.position_;


            // Main Pass
            RenderPassBeginDesc render_pass_desc = {};
            render_pass_desc.colors[0].texture = device->GetBackBuffer(frameIndex);
            render_pass_desc.colors[0].clear_color = { 0.0f, 0.2f, 0.4f, 1.0f };
            render_pass_desc.depth_stencil.texture = dsv;
            render_pass_desc.depth_stencil.clear_depth = 1.0;
            command_lists[frameIndex]->BeginEvent("Main Pass");
            command_lists[frameIndex]->UseProgram(program);
            command_lists[frameIndex]->SetViewport(0, 0, rect.width, rect.height);
            command_lists[frameIndex]->SetBlendState({true, Blend::kSrcAlpha, Blend::kInvSrcAlpha, BlendOp::kAdd});
            command_lists[frameIndex]->BeginRenderPass(render_pass_desc);

            cloth_index_buffer->Bind(*command_lists[frameIndex]);
            cloth_vertex_buffer[1-clothIdx]->BindToSlot(*command_lists[frameIndex], program.vs.ia.POSITION);
            cloth_texcoord_buffer->BindToSlot(*command_lists[frameIndex], program.vs.ia.TEXCOORD);
            command_lists[frameIndex]->Attach(program.ps.sampler.g_sampler, sampler);
            command_lists[frameIndex]->Attach(program.ps.srv.diffuseTexture, diffuseTexture);
            command_lists[frameIndex]->Attach(program.vs.cbv.Constants, program.vs.cbuffer.Constants);
            command_lists[frameIndex]->DrawIndexed(iBuf.size(), 1, 0, 0, 0);
            command_lists[frameIndex]->EndRenderPass();
            command_lists[frameIndex]->EndEvent();
        }

        {
            // Update UI
            imgui_pass.OnUpdate(); // Empty
            render_target_view = device->GetBackBuffer(frameIndex);

            ImGui::NewFrame();

            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Debug panel");

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

//                ImGui::Text("Parameters.");
//                ImGui::SliderFloat("alpha", &alpha, 0.0f, 1.0f, "alpha = %.3f");
//                ImGui::SliderFloat("setpsize", &stepSize, 0.0f, 0.01f, "setpsize = %.4f");
//                ImGui::SliderInt("maxStepCount", &maxStepCount, 1, 256);
//                ImGui::Checkbox("Using Z interpolation", &bUsingZFilter);

                ImGui::End();
            }

            ImGui::EndFrame();
        }

        {
            // ImGui Pass
            device->Wait(command_lists[frameIndex]->GetFenceValue());
            command_lists[frameIndex]->BeginEvent("ImGui Pass");
            imgui_pass.OnRender(*command_lists[frameIndex]);
            command_lists[frameIndex]->EndEvent();
        }
        command_lists[frameIndex]->Close();

        device->ExecuteCommandLists({ command_lists[device->GetFrameIndex()] });
        device->Present();


    }
    device->WaitForIdle();
    return 0;
}
