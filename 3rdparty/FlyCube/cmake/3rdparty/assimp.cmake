option (ASSIMP_BUILD_ASSIMP_TOOLS
  "If the supplementary tools for Assimp are built in addition to the library."
  OFF
)

option (ASSIMP_BUILD_TESTS
  "If the test suite for Assimp is built in addition to the library."
  OFF
)

option (ASSIMP_NO_EXPORT
  "Disable Assimp's export functionality."
  ON
)

OPTION( ASSIMP_BUILD_ZLIB
  "Build your own zlib"
  ON
)

add_subdirectory(${project_root}/3rdparty/assimp assimp EXCLUDE_FROM_ALL)

if (MSVC)
    set_target_properties(assimp PROPERTIES FOLDER "3rdparty/assimp")
    set_target_properties(zlibstatic PROPERTIES FOLDER "3rdparty/assimp")
    set_target_properties(uninstall PROPERTIES FOLDER "3rdparty/assimp")
    set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "3rdparty/assimp")
    set_target_properties(IrrXML PROPERTIES FOLDER "3rdparty/assimp")
    set_target_properties(zlib PROPERTIES FOLDER "3rdparty/assimp")
endif()
