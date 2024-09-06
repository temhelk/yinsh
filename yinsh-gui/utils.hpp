#ifndef YINSH_GUI_UTILS_HPP
#define YINSH_GUI_UTILS_HPP

#include <yinsh-gui/coords.hpp>

#include <yngine/common.hpp>
#include <raylib-cpp.hpp>

template<class... Ts>
struct variant_overloaded : Ts... { using Ts::operator()...; };

inline raylib::Vector2 to_vector2(Vec2 vec) {
    return raylib::Vector2{vec.x, vec.y};
}

inline Vec2 from_vector2(raylib::Vector2 vec) {
    return Vec2{vec.x, vec.y};
}

inline HVec2 to_hvector2(Yngine::Vec2 vec) {
    return HVec2{vec.first, vec.second};
}

inline Yngine::Vec2 from_hvector2(HVec2 vec) {
    return std::make_pair(vec.x, vec.y);
}

#endif // YINSH_GUI_UTILS_HPP
