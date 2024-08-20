add_library(extern INTERFACE)

add_subdirectory(yngine)
target_link_libraries(extern INTERFACE Yngine::Yngine)

add_subdirectory(raylib)
target_link_libraries(extern INTERFACE raylib)

add_subdirectory(raylib-cpp)
target_link_libraries(extern INTERFACE raylib_cpp)

