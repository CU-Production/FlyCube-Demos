#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <Geometry/Geometry.h>
#include <Camera/Camera.h>
#include <Utilities/FormatHelper.h>
#include <glm/gtx/transform.hpp>
#include <stdexcept>
#include <SOIL.h>

// ImGui
#include "../08ImGui/ImGuiPass.h"

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("18 - Font", settings);
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

    // ImGui
    glfwSetInputMode(app.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    auto render_target_view = device->GetBackBuffer(device->GetFrameIndex());
    ImGuiPass imgui_pass(*device, *upload_command_list, { render_target_view }, rect.width, rect.height, app.GetWindow());

    app.SubscribeEvents(&imgui_pass, &imgui_pass);

    // ImGUi fonts
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(ASSETS_PATH"fonts/Roboto/Roboto-Medium.ttf", 15.0f);
    io.Fonts->Build();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    std::shared_ptr<Resource> font_texture_view = device->CreateTexture(BindFlag::kShaderResource | BindFlag::kCopyDest, gli::format::FORMAT_RGBA8_UNORM_PACK8, 1, width, height);
    size_t num_bytes = 0;
    size_t row_bytes = 0;
    GetFormatInfo(width, height, gli::format::FORMAT_RGBA8_UNORM_PACK8, num_bytes, row_bytes);
    upload_command_list->UpdateSubresource(font_texture_view, 0, pixels, row_bytes, num_bytes);

    // Store our identifier
    io.Fonts->TexID = (void*)&font_texture_view;

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

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
            // Main Pass (no drawcall)
            RenderPassBeginDesc render_pass_desc = {};
            render_pass_desc.colors[0].texture = device->GetBackBuffer(frameIndex);
            render_pass_desc.colors[0].clear_color = { 0.0f, 0.2f, 0.4f, 1.0f };
            render_pass_desc.depth_stencil.texture = dsv;
            render_pass_desc.depth_stencil.clear_depth = 1.0;
            command_lists[frameIndex]->BeginEvent("Main Pass");
            command_lists[frameIndex]->SetViewport(0, 0, rect.width, rect.height);
            command_lists[frameIndex]->SetBlendState({true, Blend::kSrcAlpha, Blend::kInvSrcAlpha, BlendOp::kAdd});
            command_lists[frameIndex]->BeginRenderPass(render_pass_desc);
            command_lists[frameIndex]->EndRenderPass();
            command_lists[frameIndex]->EndEvent();
        }

        {
            // Update UI
            imgui_pass.OnUpdate(); // Empty
            render_target_view = device->GetBackBuffer(frameIndex);

            ImGui::NewFrame();

            {
                ImGui::Begin("Text rendering", NULL, ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoInputs);

                ImGui::TextColored({1.0, 1.0, 1.0, 1.0}, "Hello world!");
                ImGui::TextColored({0.75, 0.5, 0.0, 0.5}, "Hello world!");
                ImGui::TextColored({0.0, 0.75, 1.0, 1.0}, "Hello world!");
                ImGui::TextColored({0.0, 0.0, 0.0, 1.0}, "Hello world!");
                ImGui::PushFont(font);
                ImGui::TextColored({1.0, 1.0, 1.0, 1.0}, "Hello world!");
                ImGui::TextColored({0.75, 0.5, 0.0, 0.5}, "Hello world!");
                ImGui::TextColored({0.0, 0.75, 1.0, 1.0}, "Hello world!");
                ImGui::TextColored({0.0, 0.0, 0.0, 1.0}, "Hello world!");
                ImGui::PopFont();

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
