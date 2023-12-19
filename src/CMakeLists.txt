add_executable (BSCheat "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BSCheat PROPERTY CXX_STANDARD 20)
endif()

add_library("BananaDLL" SHARED "dllmain.cpp" "Hook.h" "Hook.cpp" "il2cpp.h" "Offsets.h")
target_link_libraries(BananaDLL "dxgi.lib;d3d11.lib")

set("PUBLIC_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(BananaDLL PUBLIC "${PUBLIC_INCLUDES}")

set("IMGUI_FOLDER" "${PUBLIC_INCLUDES}/imgui")

file(GLOB "IMGUI_SRC"
	"${IMGUI_FOLDER}/*.cpp"
	"${IMGUI_FOLDER}/*.h"
)

add_library("ImGui" STATIC "${IMGUI_SRC}")
target_link_libraries(BananaDLL ImGui)