#include "AppBox/AppBox.h"
#include "AppBox/ArgsParser.h"
#include "RenderDevice/RenderDevice.h"
#include "Texture/TextureLoader.h"

#include "ProgramRef/ShaderPS.h"
#include "ProgramRef/ShaderVS.h"

// ImGui
#include "ImGuiPass.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("08 - ImGui", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
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

    // ImGui
    auto render_target_view = device->GetBackBuffer(device->GetFrameIndex());
    ImGuiPass imgui_pass(*device, *upload_command_list, { render_target_view }, rect.width, rect.height, app.GetWindow());

    app.SubscribeEvents(&imgui_pass, &imgui_pass);

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

    // IMGUI Demo: Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!app.PollEvents())
    {
        int32_t frameIndex = device->GetFrameIndex();
        command_lists[frameIndex]->Reset();
        {
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
            command_lists[frameIndex]->Attach(program.ps.srv.diffuseTexture, diffuseTexture);
            command_lists[frameIndex]->Attach(program.ps.sampler.g_sampler, g_sampler);
            command_lists[frameIndex]->BeginRenderPass(render_pass_desc);
            command_lists[frameIndex]->DrawIndexed(6, 0, 0);
            command_lists[frameIndex]->EndRenderPass();
            command_lists[frameIndex]->EndEvent();
        }

        {
            // Update UI
            imgui_pass.OnUpdate(); // Empty
            render_target_view = device->GetBackBuffer(frameIndex);

            ImGui::NewFrame();
            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

                ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
                ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
                ImGui::Checkbox("Another Window", &show_another_window);

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }

            // 3. Show another simple window.
            if (show_another_window)
            {
                ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::Text("Hello from another window!");
                if (ImGui::Button("Close Me"))
                    show_another_window = false;
                ImGui::End();
            }
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
