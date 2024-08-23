#ifndef YINSH_GUI_COORDS_HPP
#define YINSH_GUI_COORDS_HPP

#include <yngine/common.hpp>

#include <cstdint>
#include <vector>

const float SQRT_THREE = 1.73205080756887729353f;

struct HVec2;
struct HVec3;

struct Vec2 {
    float x, y;

    HVec2 from_world() const;
};

struct Vec3 {
    float x, y, z;
};

// Hexagonal 2d vector with integer coordinates
struct HVec2 {
    int32_t x, y;

    const static std::vector<HVec2> Directions;

    HVec2();
    HVec2(int32_t x, int32_t y);
    HVec2(HVec3 vec);

    HVec2 operator+(const HVec2 rhs) const;
    HVec2& operator+=(const HVec2 rhs);
    HVec2 operator-(const HVec2 rhs) const;
    HVec2 operator-() const;
    HVec2 operator*(int32_t rhs) const;
    bool operator==(const HVec2 rhs) const;

    Vec2 to_world() const;

    static HVec2 up();

    static HVec2 from_direction(Yngine::Direction direction);
};

HVec2 operator*(int32_t lhs, HVec2 rhs);

// Hexagonal 3d vector with integer coordinates
struct HVec3 {
    int32_t x, y, z;

    HVec3(int32_t x, int32_t y, int32_t z);
    HVec3(HVec2 vec);

    bool operator==(const HVec3 rhs) const;
    HVec3 operator-(const HVec3 rhs) const;
    HVec3 operator/(const int32_t rhs) const;
    HVec3& operator/=(const int32_t rhs);

    int32_t length() const;

    HVec3 closest_straight_line() const;

    Yngine::Direction direction_to(HVec3 to) const;
};

#endif // YINSH_GUI_COORDS_HPP
