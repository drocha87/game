cmake_minimum_required (VERSION 3.12)

project (Game)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable (Game
  ${SOURCES}
  vendor/imgui-1.91.9b/imgui.cpp
  vendor/imgui-1.91.9b/imgui_draw.cpp
  vendor/imgui-1.91.9b/imgui_tables.cpp
  vendor/imgui-1.91.9b/imgui_widgets.cpp
  vendor/imgui-1.91.9b/backends/imgui_impl_sdl3.cpp
  vendor/imgui-1.91.9b/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(Game PRIVATE src)

set_property(TARGET Game PROPERTY CXX_STANDARD 20)

target_include_directories(Game PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/FastNoise2/include
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL3-3.2.16/include
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL3_image-3.2.4/include
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL3_ttf-3.1.0/include
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui-1.91.9b/
)

target_link_directories(Game PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL3-3.2.16/lib/x64
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL3_image-3.2.4/lib/x64
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL3_ttf-3.1.0/lib/x64
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/FastNoise2/lib
)

target_link_libraries(Game PRIVATE SDL3 SDL3_ttf SDL3_image FastNoiseD)

# List of DLLs and their source paths
set(DLLS
  "${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL3-3.2.16/lib/x64/SDL3.dll"
  "${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL3_image-3.2.4/lib/x64/SDL3_image.dll"
  "${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL3_ttf-3.1.0/lib/x64/SDL3_ttf.dll"
  "${CMAKE_CURRENT_SOURCE_DIR}/vendor/FastNoise2/bin/FastNoiseD.dll"
  "${CMAKE_CURRENT_SOURCE_DIR}/vendor/FastNoise2/bin/FastNoise.dll"
  "${CMAKE_CURRENT_SOURCE_DIR}/font.ttf"
)

# Copy all DLLs to the executable output directory after build
foreach(DLL ${DLLS})
  add_custom_command(TARGET Game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DLL}" "$<TARGET_FILE_DIR:Game>"
  )
endforeach()

# === Copy assets folder ===
add_custom_command(TARGET Game POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:Game>/assets"
)
