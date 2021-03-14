set(target 05RayTracing)

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}")
set(shaders_path "${assets_path}/shaders/05RayTracing")

set(sources
    ${source_path}/main.cpp
)

set(lib_shaders
    ${shaders_path}/RayTracing.hlsl
)

set_property(SOURCE ${lib_shaders} PROPERTY VS_SHADER_ENTRYPOINT " ")
set_property(SOURCE ${lib_shaders} PROPERTY VS_SHADER_TYPE Library)
set_property(SOURCE ${lib_shaders} PROPERTY VS_SHADER_MODEL 6.5)
set_property(SOURCE ${lib_shaders} PROPERTY VS_SHADER_FLAGS "/Zi /auto-binding-space 0")

set(shaders_files ${lib_shaders})

include(${project_root}/3rdparty/FlyCube/cmake/shader_parser.cmake)

source_group("Shader Files" FILES ${shaders_files})
source_group("Shader Ref Files" FILES ${shaders_ref})

add_executable(${target} WIN32 ${headers} ${sources} ${shaders_files} ${shaders_ref})
gen_shaders_ref(${target} "${shaders_files}" shaders_ref)

set_target_properties(${target} PROPERTIES
                        LINK_FLAGS "/ENTRY:mainCRTStartup")

target_include_directories(${target}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ${CMAKE_BINARY_DIR}/gen/${target}
)

target_link_libraries(${target}
    AppBox
    RenderDevice
    Texture
)

set_target_properties(${target} PROPERTIES FOLDER "Apps")

install(TARGETS ${target})
