#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Geometry/Geometry.h>
#include <Camera/Camera.h>
#include <Utilities/FormatHelper.h>
#include <glm/gtx/transform.hpp>
#include <stdexcept>
#include <SOIL.h>

#include "ProgramRef/Shader_PS.h"
#include "ProgramRef/Shader_VS.h"
#include "ProgramRef/VolumeTexGen.h"

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
    AppBox app("17 - VolumeTexture", settings);
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

    // Volume(Texture3D)
//    std::shared_ptr<Resource> volumeTextureSlice = CreateTexture(*device, *upload_command_list, ASSETS_PATH"textures/volume/volume_pig_y.png");
    std::shared_ptr<Resource> volumeTextureSlice = CreateTexture(*device, *upload_command_list, ASSETS_PATH"textures/volume/volume_rubbertoy_y.png");

    const static int uavWidth = 128;
    const static int uavHeight = 128;
    const static int uavDepth = 128;
    std::shared_ptr<Resource> uav = device->CreateTexture(BindFlag::kUnorderedAccess | BindFlag::kShaderResource | BindFlag::kCopySource,
                                                          device->GetFormat(), 1, uavWidth, uavHeight, uavDepth);

    Camera camera;
    camera.SetCameraPos(glm::vec3(0.0, 0.0, 1.0));
    camera.SetViewport(rect.width, rect.height);
    camera.movement_speed_ = 1.0f;
    camera.z_near_ = 0.001f;

    Model model(*device, *upload_command_list, ASSETS_PATH"model/cube.obj");
    model.matrix = glm::scale(glm::vec3(0.1f)) * glm::translate(glm::vec3(0.0f, 0.0f, 0.0f)) * glm::rotate(glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // ImGui
    auto render_target_view = device->GetBackBuffer(device->GetFrameIndex());
    ImGuiPass imgui_pass(*device, *upload_command_list, { render_target_view }, rect.width, rect.height, app.GetWindow());

    app.SubscribeEvents(&imgui_pass, &imgui_pass);

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<Shader_PS, Shader_VS> program(*device);
    ProgramHolder<VolumeTexGen> programCS(*device);

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
//    glfwSetKeyCallback(app.GetWindow(), [](GLFWwindow* window, int key, int scancode, int action, int mods)
//    {
//        if (action == GLFW_PRESS)
//            keys[key] = true;
//        else if (action == GLFW_RELEASE)
//            keys[key] = false;
//    });

    float alpha = 0.2f;
    float stepSize = 0.01;

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
            // Generate volume texture from 2d slice
            command_lists[frameIndex]->BeginEvent("Gen volume texture from 2d slices");
            command_lists[frameIndex]->UseProgram(programCS);
            command_lists[frameIndex]->Attach(programCS.cs.srv.volumeSlicesTex, volumeTextureSlice);
            command_lists[frameIndex]->Attach(programCS.cs.uav.uav, uav);
            command_lists[frameIndex]->Dispatch(32, 32, 32); // (128 / 4)
            command_lists[frameIndex]->EndEvent();
        }

        {
            program.vs.cbuffer.Constants.World = glm::transpose(model.matrix);
            program.vs.cbuffer.Constants.WorldView = program.vs.cbuffer.Constants.World * glm::transpose(camera.GetViewMatrix());
            program.vs.cbuffer.Constants.WorldViewProj = program.vs.cbuffer.Constants.WorldView * glm::transpose(camera.GetProjectionMatrix());
            program.vs.cbuffer.Constants.CameraPos = camera.position_;

            program.ps.cbuffer.Settings.volumeAlpha = alpha;
            program.ps.cbuffer.Settings.stepSize = stepSize;
            program.ps.cbuffer.Constants.WorldToObj = glm::inverse(glm::transpose(model.matrix));

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
            for (auto& range : model.ia.ranges)
            {
                model.ia.indices.Bind(*command_lists[frameIndex]);
                model.ia.positions.BindToSlot(*command_lists[frameIndex], program.vs.ia.POSITION);
                model.ia.texcoords.BindToSlot(*command_lists[frameIndex], program.vs.ia.TEXCOORD);
                command_lists[frameIndex]->Attach(program.vs.cbv.Constants, program.vs.cbuffer.Constants);
                command_lists[frameIndex]->Attach(program.ps.cbv.Constants, program.ps.cbuffer.Constants);
                command_lists[frameIndex]->Attach(program.ps.cbv.Settings, program.ps.cbuffer.Settings);
                command_lists[frameIndex]->Attach(program.ps.sampler.g_sampler, sampler);
                command_lists[frameIndex]->Attach(program.ps.srv.volumeTexture, uav);
                command_lists[frameIndex]->DrawIndexed(range.index_count, 1, range.start_index_location, range.base_vertex_location, 0);
            }
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

                ImGui::Text("Parameters.");
                ImGui::SliderFloat("alpha", &alpha, 0.0f, 1.0f, "ratio = %.3f");
                ImGui::SliderFloat("setpsize", &stepSize, 0.0f, 0.01f, "ratio = %.4f");

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
