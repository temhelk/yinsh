add_library(
    imgui OBJECT

    extern/imgui/imgui.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui.h
    extern/imgui/imgui_internal.h
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/imstb_rectpack.h
    extern/imgui/imstb_textedit.h
    extern/imgui/imstb_truetype.h

    extern/rlImGui/rlImGui.cpp
    extern/rlImGui/rlImGui.h
    extern/rlImGui/rlImGuiColors.h
    extern/rlImGui/imgui_impl_raylib.h
)

target_include_directories(
    imgui PUBLIC
    extern/imgui/
    extern/imgui/backends/
    extern/rlImGui/
    extern/raylib/src
)
