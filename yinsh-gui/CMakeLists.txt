add_executable(
    Yinsh-gui
    main.cpp
    game.cpp game.hpp
    board.cpp board.hpp
    coords.cpp coords.hpp
    system.cpp system.hpp
    utils.hpp
)

# set_target_properties(Yinsh-gui PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)

target_compile_features(Yinsh-gui PUBLIC cxx_std_20)

set_target_properties(
    Yinsh-gui PROPERTIES
    CXX_EXTENSIONS OFF
    INTERPROCEDURAL_OPTIMIZATION $<$<CONFIG:Debug>:FALSE:TRUE>
)

target_compile_definitions(
    Yinsh-gui
    PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

if(WIN32)
    target_link_libraries(Yinsh-gui PRIVATE -static-libgcc -static-libstdc++)
endif()

if(EMSCRIPTEN)
    set_target_properties(
        Yinsh-gui PROPERTIES
        SUFFIX .html
    )

    target_link_options(Yinsh-gui PRIVATE --shell-file ${PROJECT_SOURCE_DIR}/extern/raylib/src/minshell.html)
endif()

target_include_directories(Yinsh-gui PUBLIC ${PROJECT_SOURCE_DIR})
