add_subdirectory(${project_root}/3rdparty/SPIRV-Cross EXCLUDE_FROM_ALL)
set_target_properties(spirv-cross PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-core PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-cpp PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-msl PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-reflect PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-util PROPERTIES FOLDER "3rdparty/spirv-cross")
set_target_properties(spirv-cross-c PROPERTIES FOLDER "3rdparty/spirv-cross")
