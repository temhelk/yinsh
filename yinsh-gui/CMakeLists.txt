add_executable(
    Yinsh-gui
    main.cpp
    game.cpp game.hpp
    board.cpp board.hpp
    coords.cpp coords.hpp
    utils.hpp
)

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

target_include_directories(Yinsh-gui PUBLIC ${PROJECT_SOURCE_DIR})
