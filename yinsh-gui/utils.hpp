#ifndef YINSH_GUI_UTILS_HPP
#define YINSH_GUI_UTILS_HPP

#include <yinsh-gui/coords.hpp>

#include <raylib-cpp.hpp>

inline raylib::Vector2 to_vector2(Vec2 vec) {
    return raylib::Vector2{vec.x, vec.y};
}

inline Vec2 from_vector2(raylib::Vector2 vec) {
    return Vec2{vec.x, vec.y};
}

#endif // YINSH_GUI_UTILS_HPP
