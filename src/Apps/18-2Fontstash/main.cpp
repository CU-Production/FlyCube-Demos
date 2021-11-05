#include <AppBox/AppBox.h>
#include <AppBox/ArgsParser.h>
#include <RenderDevice/RenderDevice.h>
#include "Texture/TextureLoader.h"
#include <Utilities/FormatHelper.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Geometry/IABuffer.h>

#include <ProgramRef/Shader_PS.h>
#include <ProgramRef/Shader_VS.h>

#pragma region FONTSTASH_IMPL

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define FONTSTASH_IMPLEMENTATION
#define FONS_USE_FREETYPE
#include "fontstash.h"

#ifndef _FC_UNUSED
    #define _FC_UNUSED(x) (void)(x)
#endif

#if 1
struct FCFONScontext {
    std::shared_ptr<Resource> fontTexture;
    gli::format fontTexFormat;
    bool bFontTextureNeedUpdate;
    std::shared_ptr<RenderCommandList> cmd_list;
    std::shared_ptr<RenderDevice> device;
    int width, height;

    std::array<std::unique_ptr<IAVertexBuffer>, 3> positions_buffer;
    std::array<std::unique_ptr<IAVertexBuffer>, 3> texcoords_buffer;
    std::array<std::unique_ptr<IAVertexBuffer>, 3> colors_buffer;
    std::array<std::unique_ptr<IAIndexBuffer>, 3> indices_buffer;
    int frameIndex;

    uint32_t posSlot;
    uint32_t uvSlot;
    uint32_t colorSlot;
};
typedef struct GLFONScontext GLFONScontext;

static int FCfons__renderCreate(void* userPtr, int width, int height)
{
    FCFONScontext* fc = (FCFONScontext*)userPtr;
    // Create may be called multiple times, delete existing texture.
    if (fc->fontTexture != nullptr) {
        fc->fontTexture = nullptr;
    }

    fc->width = width;
    fc->height = height;

    fc->fontTexFormat = gli::format::FORMAT_R8_UNORM_PACK8;
    fc->fontTexture = fc->device->CreateTexture(BindFlag::kShaderResource | BindFlag::kCopyDest, fc->fontTexFormat, 1, width, height);

    return 1;
}

static int FCfons__renderResize(void* userPtr, int width, int height)
{
    // Reuse create to resize too.
    return FCfons__renderCreate(userPtr, width, height);
}

static void FCfons__renderUpdate(void* userPtr, int* rect, const unsigned char* data)
{
    _FC_UNUSED(rect);
    _FC_UNUSED(data);
    FCFONScontext* fc = (FCFONScontext*)userPtr;
    fc->bFontTextureNeedUpdate = true;
}

const static float inv255 = 1.0 / 255.0;
static void FCfons__renderDraw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts)
{
    FCFONScontext* fc = (FCFONScontext*)userPtr;
    if (fc->fontTexture == nullptr) return;

    std::vector<uint32_t> ibuf(nverts);
    std::vector<glm::vec4> colorBuf(nverts);
    std::vector<glm::vec2> posBuf(nverts);
    std::vector<glm::vec2> uvBuf(nverts);
    for (int i = 0; i < nverts; i++)
    {
        // index
        ibuf[i] = i;
        // color
        uint8_t r; uint8_t g; uint8_t b; uint8_t a;
        r = colors[i] >>  0;
        g = colors[i] >>  8;
        b = colors[i] >> 16;
        a = colors[i] >> 24;
        colorBuf[i] = glm::vec4(r * inv255, g * inv255, b * inv255, a * inv255);
        // pos
        posBuf[i] = glm::vec2(verts[i * 2 + 0], verts[i * 2 + 1]);
        //uv
        uvBuf[i] = glm::vec2(tcoords[i * 2 + 0], tcoords[i * 2 + 1]);
    }

    fc->positions_buffer[fc->frameIndex].reset(new IAVertexBuffer(*fc->device, *fc->cmd_list, posBuf));
    fc->texcoords_buffer[fc->frameIndex].reset(new IAVertexBuffer(*fc->device, *fc->cmd_list, uvBuf));
    fc->colors_buffer[fc->frameIndex].reset(new IAVertexBuffer(*fc->device, *fc->cmd_list, colorBuf));
    fc->indices_buffer[fc->frameIndex].reset(new IAIndexBuffer(*fc->device, *fc->cmd_list, ibuf, gli::format::FORMAT_R32_UINT_PACK32));

    fc->indices_buffer[fc->frameIndex]->Bind(*fc->cmd_list);
    fc->positions_buffer[fc->frameIndex]->BindToSlot(*fc->cmd_list, fc->posSlot);
    fc->texcoords_buffer[fc->frameIndex]->BindToSlot(*fc->cmd_list, fc->uvSlot);
    fc->colors_buffer[fc->frameIndex]->BindToSlot(*fc->cmd_list, fc->colorSlot);

    fc->cmd_list->DrawIndexed(nverts, 1, 0, 0, 0);
}

static void FCfons__renderDelete(void* userPtr)
{
    FCFONScontext* fc = (FCFONScontext*)userPtr;
    free(fc);
}

void sfons_flush(FONScontext* ctx) {
    assert(ctx && ctx->params.userPtr);
    FCFONScontext* fc = (FCFONScontext*)ctx->params.userPtr;
    if (fc->bFontTextureNeedUpdate)
    {
        // Update fontTexture
        size_t row_bytes = 0;
        size_t num_bytes = 0;
        GetFormatInfo(fc->width, fc->height, fc->fontTexFormat, num_bytes, row_bytes);
        fc->cmd_list->UpdateSubresource(fc->fontTexture, 0, ctx->texData, row_bytes, num_bytes);
        fc->bFontTextureNeedUpdate = false;
    }
}


unsigned int FCfonsRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    return (r) | (g << 8) | (b << 16) | (a << 24);
}
#endif
#pragma endregion FONTSTASH_IMPL

int main(int argc, char* argv[])
{
    Settings settings = ParseArgs(argc, argv);
    AppBox app("18.2 - Fontstash", settings);
    AppRect rect = app.GetAppRect();

    std::shared_ptr<RenderDevice> device = CreateRenderDevice(settings, app.GetWindow());
    app.SetGpuName(device->GetGpuName());

    std::shared_ptr<RenderCommandList> upload_command_list = device->CreateRenderCommandList();
    std::vector<uint32_t> ibuf = { 0, 1, 2, 0, 2, 3 };
    std::shared_ptr<Resource> index = device->CreateBuffer(BindFlag::kIndexBuffer | BindFlag::kCopyDest, sizeof(uint32_t) * ibuf.size());
    upload_command_list->UpdateSubresource(index, 0, ibuf.data(), 0, 0);
    std::vector<glm::vec2> pbuf = {
            glm::vec2(-0.5,  0.5),
            glm::vec2( 0.5,  0.5),
            glm::vec2( 0.5, -0.5),
            glm::vec2(-0.5, -0.5),
    };
    std::vector<glm::vec2> uvbuf = {
            glm::vec2(0.0, 0.0),
            glm::vec2(1.0, 0.0),
            glm::vec2(1.0, 1.0),
            glm::vec2(0.0, 1.0),
    };
    std::shared_ptr<Resource> pos = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec2) * pbuf.size());
    upload_command_list->UpdateSubresource(pos, 0, pbuf.data(), 0, 0);
    std::shared_ptr<Resource> uv = device->CreateBuffer(BindFlag::kVertexBuffer | BindFlag::kCopyDest, sizeof(glm::vec2) * uvbuf.size());
    upload_command_list->UpdateSubresource(uv, 0, uvbuf.data(), 0, 0);

    std::shared_ptr<Resource> g_sampler = device->CreateSampler({
        SamplerFilter::kAnisotropic,
        SamplerTextureAddressMode::kWrap,
        SamplerComparisonFunc::kNever
    });
    std::shared_ptr<Resource> diffuseTexture = CreateTexture(*device, *upload_command_list, ASSETS_PATH"model/export3dcoat/export3dcoat_lambert3SG_color.dds");

    upload_command_list->Close();
    device->ExecuteCommandLists({ upload_command_list });

    ProgramHolder<Shader_PS, Shader_VS> program(*device);
    //program.ps.cbuffer.Settings.color = glm::vec4(1, 0, 0, 1);

    // Fontstash demo
    int debug = 1;

    int fontNormal = FONS_INVALID;
    int fontItalic = FONS_INVALID;
    int fontBold = FONS_INVALID;
    int fontJapanese = FONS_INVALID;

    float sx, sy, dx, dy, lh = 0;
    int width, height;
    unsigned int white,black,brown,blue;

    FONScontext* fs = nullptr;
    FCFONScontext* fc;

    int fontTexWidth = 512;
    int fontTexHeight = 512;
    int fontTexFlags = FONS_ZERO_TOPLEFT;
    {
        FONSparams params;

        fc = (FCFONScontext*)malloc(sizeof(FCFONScontext));
        memset(fc, 0, sizeof(FCFONScontext));

        fc->device = device;

        fc->posSlot = program.vs.ia.POSITION;
        fc->uvSlot = program.vs.ia.TEXCOORD;
        fc->colorSlot = program.vs.ia.COLOR;

        memset(&params, 0, sizeof(params));
        params.width = fontTexWidth;
        params.height = fontTexHeight;
        params.flags = (unsigned char)fontTexFlags;
        params.renderCreate = FCfons__renderCreate;
        params.renderResize = FCfons__renderResize;
        params.renderUpdate = FCfons__renderUpdate;
        params.renderDraw = FCfons__renderDraw;
        params.renderDelete = FCfons__renderDelete;
        params.userPtr = fc;

        fs = fonsCreateInternal(&params);
    }
    {
        fontNormal = fonsAddFont(fs, "sans", ASSETS_PATH"fonts/DroidSerif/DroidSerif-Regular.ttf");
        if (fontNormal == FONS_INVALID) {
            printf("Could not add font normal.\n");
            return -1;
        }
        fontItalic = fonsAddFont(fs, "sans-italic", ASSETS_PATH"fonts/DroidSerif/DroidSerif-Italic.ttf");
        if (fontItalic == FONS_INVALID) {
            printf("Could not add font italic.\n");
            return -1;
        }
        fontBold = fonsAddFont(fs, "sans-bold", ASSETS_PATH"fonts/DroidSerif/DroidSerif-Bold.ttf");
        if (fontBold == FONS_INVALID) {
            printf("Could not add font bold.\n");
            return -1;
        }
        fontJapanese = fonsAddFont(fs, "sans-jp", ASSETS_PATH"fonts/DroidSerif/DroidSansJapanese.ttf");
        if (fontJapanese == FONS_INVALID) {
            printf("Could not add font japanese.\n");
            return -1;
        }
    }

    // Rendering

    std::vector<std::shared_ptr<RenderCommandList>> command_lists;
    for (uint32_t i = 0; i < settings.frame_count; ++i)
    {
        decltype(auto) command_list = device->CreateRenderCommandList();
        command_lists.emplace_back(command_list);
    }

    while (!app.PollEvents())
    {
        int frameIndex = device->GetFrameIndex();
        device->Wait(command_lists[frameIndex]->GetFenceValue());
        command_lists[frameIndex]->Reset();

        fc->frameIndex = frameIndex;
        fc->cmd_list = command_lists[frameIndex];

        RenderPassBeginDesc render_pass_desc = {};
        render_pass_desc.colors[0].texture = device->GetBackBuffer(frameIndex);
        render_pass_desc.colors[0].clear_color = { 0.0f, 0.2f, 0.4f, 1.0f };

        program.vs.cbuffer.vertexBuffer.ProjectionMatrix = glm::ortho(0.0f, 1.0f * rect.width, 1.0f * rect.height, 0.0f);

        command_lists[frameIndex]->UseProgram(program);
        command_lists[frameIndex]->SetViewport(0, 0, rect.width, rect.height);
        command_lists[frameIndex]->IASetIndexBuffer(index, gli::format::FORMAT_R32_UINT_PACK32);
        command_lists[frameIndex]->IASetVertexBuffer(program.vs.ia.POSITION, pos);
        command_lists[frameIndex]->IASetVertexBuffer(program.vs.ia.TEXCOORD, uv);
        command_lists[frameIndex]->Attach(program.ps.srv.diffuseTexture, fc->fontTexture);
        command_lists[frameIndex]->Attach(program.ps.sampler.g_sampler, g_sampler);
        command_lists[frameIndex]->Attach(program.vs.cbv.vertexBuffer, program.vs.cbuffer.vertexBuffer);
        command_lists[frameIndex]->BeginRenderPass(render_pass_desc);

#if 1
        command_lists[frameIndex]->SetBlendState({
            true,
            Blend::kSrcAlpha,
            Blend::kInvSrcAlpha,
            BlendOp::kAdd,
            Blend::kInvSrcAlpha,
            Blend::kZero,
            BlendOp::kAdd
        });

        command_lists[frameIndex]->SetRasterizeState({ FillMode::kSolid, CullMode::kNone });
        command_lists[frameIndex]->SetDepthStencilState({ false, ComparisonFunc::kLessEqual });

        {
            sfons_flush(fs);

            white = FCfonsRGBA(255, 255, 255, 255);
            brown = FCfonsRGBA(192, 128, 0, 128);
            blue = FCfonsRGBA(0, 192, 255, 255);
            black = FCfonsRGBA(0, 0, 0, 255);

            sx = 50; sy = 50;

            dx = sx; dy = sy;


            fonsClearState(fs);

            fonsSetSize(fs, 124.0f);
            fonsSetFont(fs, fontNormal);
            fonsVertMetrics(fs, NULL, NULL, &lh);
            dx = sx;
            dy += lh;

            fonsSetSize(fs, 124.0f);
            fonsSetFont(fs, fontNormal);
            fonsSetColor(fs, white);
            dx = fonsDrawText(fs, dx,dy,"The quick ",NULL);

            fonsSetSize(fs, 48.0f);
            fonsSetFont(fs, fontItalic);
            fonsSetColor(fs, brown);
            dx = fonsDrawText(fs, dx,dy,"brown ",NULL);

            fonsSetSize(fs, 24.0f);
            fonsSetFont(fs, fontNormal);
            fonsSetColor(fs, white);
            dx = fonsDrawText(fs, dx,dy,"fox ",NULL);

            fonsVertMetrics(fs, NULL, NULL, &lh);
            dx = sx;
            dy += lh*1.2f;
            fonsSetFont(fs, fontItalic);
            dx = fonsDrawText(fs, dx,dy,"jumps over ",NULL);
            fonsSetFont(fs, fontBold);
            dx = fonsDrawText(fs, dx,dy,"the lazy ",NULL);
            fonsSetFont(fs, fontNormal);
            dx = fonsDrawText(fs, dx,dy,"dog.",NULL);

            dx = sx;
            dy += lh*1.2f;
            fonsSetSize(fs, 12.0f);
            fonsSetFont(fs, fontNormal);
            fonsSetColor(fs, blue);
            fonsDrawText(fs, dx,dy,"Now is the time for all good men to come to the aid of the party.",NULL);

            fonsVertMetrics(fs, NULL,NULL,&lh);
            dx = sx;
            dy += lh*1.2f*2;
            fonsSetSize(fs, 18.0f);
            fonsSetFont(fs, fontItalic);
            fonsSetColor(fs, white);
            fonsDrawText(fs, dx,dy,"Ég get etið gler án þess að meiða mig.",NULL);

            fonsVertMetrics(fs, NULL,NULL,&lh);
            dx = sx;
            dy += lh*1.2f;
            fonsSetFont(fs, fontJapanese);
            const char* japChar = R"(
"私はガラスを食べられます。それは私を傷つけません。"
)";
            fonsDrawText(fs, dx,dy, japChar, NULL);

            // Font alignment
            fonsSetSize(fs, 18.0f);
            fonsSetFont(fs, fontNormal);
            fonsSetColor(fs, white);

            dx = 50; dy = 350;
            fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
            dx = fonsDrawText(fs, dx,dy,"Top",NULL);
            dx += 10;
            fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_MIDDLE);
            dx = fonsDrawText(fs, dx,dy,"Middle",NULL);
            dx += 10;
            fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_BASELINE);
            dx = fonsDrawText(fs, dx,dy,"Baseline",NULL);
            dx += 10;
            fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_BOTTOM);
            fonsDrawText(fs, dx,dy,"Bottom",NULL);

            dx = 150; dy = 400;
            fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_BASELINE);
            fonsDrawText(fs, dx,dy,"Left",NULL);
            dy += 30;
            fonsSetAlign(fs, FONS_ALIGN_CENTER | FONS_ALIGN_BASELINE);
            fonsDrawText(fs, dx,dy,"Center",NULL);
            dy += 30;
            fonsSetAlign(fs, FONS_ALIGN_RIGHT | FONS_ALIGN_BASELINE);
            fonsDrawText(fs, dx,dy,"Right",NULL);

            // Blur
            dx = 500; dy = 350;
            fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_BASELINE);

            fonsSetSize(fs, 60.0f);
            fonsSetFont(fs, fontItalic);
            fonsSetColor(fs, white);
            fonsSetSpacing(fs, 5.0f);
            fonsSetBlur(fs, 10.0f);
            fonsDrawText(fs, dx,dy,"Blurry...",NULL);

            dy += 50.0f;

            fonsSetSize(fs, 18.0f);
            fonsSetFont(fs, fontBold);
            fonsSetColor(fs, black);
            fonsSetSpacing(fs, 0.0f);
            fonsSetBlur(fs, 3.0f);
            fonsDrawText(fs, dx,dy+2,"DROP THAT SHADOW",NULL);

            fonsSetColor(fs, white);
            fonsSetBlur(fs, 0);
            fonsDrawText(fs, dx,dy,"DROP THAT SHADOW",NULL);

            if (debug)
                fonsDrawDebug(fs, 760.0, 50.0);
        }
#endif

        command_lists[frameIndex]->EndRenderPass();
        command_lists[frameIndex]->Close();

        device->ExecuteCommandLists({ command_lists[frameIndex] });
        device->Present();
    }

    fonsDeleteInternal(fs);

    return 0;
}
