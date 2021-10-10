set(target 17-3VolumeTexture2D)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(shaders_path "${assets_path}/shaders")

set(sources
    ${source_path}/main.cpp
    ${source_path}/../08ImGui/ImGuiPass.cpp
)

set(pixel_shaders
    ${shaders_path}/17-3VolumeTexture2D/Shader_PS.hlsl
    ${shaders_path}/08ImGui/ImGuiPass_PS.hlsl
)

set(vertex_shaders
    ${shaders_path}/17-3VolumeTexture2D/Shader_VS.hlsl
    ${shaders_path}/08ImGui/ImGuiPass_VS.hlsl
)

set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_ENTRYPOINT mainPS)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_MODEL 6.0)
set_property(SOURCE ${pixel_shaders} PROPERTY VS_SHADER_FLAGS "/Zi /auto-binding-space 0")

set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_ENTRYPOINT mainVS)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_MODEL 6.0)
set_property(SOURCE ${vertex_shaders} PROPERTY VS_SHADER_FLAGS "/Zi /auto-binding-space 0")

set(shaders_files ${pixel_shaders} ${vertex_shaders})

include(${project_root}/3rdparty/FlyCube/cmake/shader_parser.cmake)

source_group("Shader Files" FILES ${shaders_files})
source_group("Shader Ref Files" FILES ${shaders_ref})

add_executable(${target} WIN32 ${headers} ${sources} ${shaders_files} ${shaders_ref})
gen_shaders_ref(${target} "${shaders_files}" shaders_ref)

if (WIN32)
    set_target_properties(${target} PROPERTIES
                          LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ${CMAKE_BINARY_DIR}/gen/${target}
)

target_link_libraries(${target}
    AppBox
    RenderDevice
    Geometry
    Texture
    Camera
    imgui
)

set_target_properties(${target} PROPERTIES FOLDER "Apps")

install(TARGETS ${target})
