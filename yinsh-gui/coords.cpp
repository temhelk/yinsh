#include <yinsh-gui/coords.hpp>

#include <cmath>
#include <numbers>

const std::vector<HVec2> HVec2::Directions = {
    HVec2{1, 0}, HVec2{0, 1}, HVec2{-1, 1},
    HVec2{-1, 0}, HVec2{0, -1}, HVec2{1, -1}
};

HVec2::HVec2() : x{0}, y{0} {}

HVec2::HVec2(int32_t x, int32_t y) : x{x}, y{y} {}

HVec2::HVec2(HVec3 vec) : x{vec.x}, y{vec.y} {}

HVec2 HVec2::operator+(const HVec2 rhs) const {
    return HVec2{this->x + rhs.x, this->y + rhs.y};
}

HVec2 HVec2::operator-(const HVec2 rhs) const {
    return HVec2{this->x - rhs.x, this->y - rhs.y};
}

HVec2 HVec2::operator-() const {
    return HVec2{-this->x, -this->y};
}

bool HVec2::operator==(const HVec2 rhs) const {
    return this->x == rhs.x && this->y == rhs.y;
}

HVec2& HVec2::operator+=(const HVec2 rhs) {
    this->x += rhs.x;
    this->y += rhs.y;
    return *this;
}

HVec2 HVec2::operator*(int32_t rhs) const {
    return HVec2{this->x * rhs, this->y * rhs};
}

HVec2 operator*(int32_t lhs, HVec2 rhs) {
    return rhs * lhs;
}

Vec2 HVec2::to_world() const {
    return Vec2{
        (this->x + this->y) * (std::numbers::sqrt3_v<float> / 2.f),
        (this->x - this->y) / 2.f
    };
}

HVec2 HVec2::up() {
    return HVec2{-1, 1};
}

HVec2 HVec2::from_direction(Yngine::Direction direction) {
    switch (direction) {
    case Yngine::Direction::SE:
        return HVec2{1, 0};
    case Yngine::Direction::NE:
        return HVec2{0, 1};
    case Yngine::Direction::N:
        return HVec2{-1, 1};
    case Yngine::Direction::NW:
        return HVec2{-1, 0};
    case Yngine::Direction::SW:
        return HVec2{0, -1};
    case Yngine::Direction::S:
        return HVec2{1, -1};
    default:
        abort();
    }
}

HVec2 Vec2::from_world() const {
    auto frac = Vec3{
        this->x / std::numbers::sqrt3_v<float> + this->y,
        this->x / std::numbers::sqrt3_v<float> - this->y,
        0
    };
    frac.z = -frac.x - frac.y;

    const auto rounded = Vec3{
        std::round(frac.x),
        std::round(frac.y),
        std::round(frac.z),
    };

    const auto x_diff = std::abs(frac.x - rounded.x);
    const auto y_diff = std::abs(frac.y - rounded.y);
    const auto z_diff = std::abs(frac.z - rounded.z);

    auto result = HVec3{
        static_cast<int32_t>(std::round(frac.x)),
        static_cast<int32_t>(std::round(frac.y)),
        static_cast<int32_t>(std::round(frac.z)),
    };

    if (x_diff > y_diff && x_diff > z_diff) {
        result.x = -result.y - result.z;
    } else if (y_diff > z_diff) {
        result.y = -result.x - result.z;
    } else {
        result.z = -result.x - result.y;
    }

    return result;
}

HVec3::HVec3(int32_t x, int32_t y, int32_t z) : x{x}, y{y}, z{z} {}

HVec3::HVec3(HVec2 vec) : x{vec.x}, y{vec.y}, z{-vec.x-vec.y} {}

HVec3& HVec3::operator/=(const int32_t rhs) {
    this->x /= rhs;
    this->y /= rhs;
    this->z /= rhs;
    return *this;
}

bool HVec3::operator==(const HVec3 rhs) const {
    return
        this->x == rhs.x &&
        this->y == rhs.y &&
        this->z == rhs.z;
}

HVec3 HVec3::operator-(const HVec3 rhs) const {
    return HVec3{
        this->x - rhs.x,
        this->y - rhs.y,
        this->z - rhs.z
    };
}

HVec3 HVec3::operator/(const int32_t rhs) const {
    return HVec3{this->x / rhs, this->y / rhs, this->z / rhs};
}

int32_t HVec3::length() const {
    const auto total =
        std::abs(this->x) +
        std::abs(this->y) +
        std::abs(this->z);
    return total / 2;
}

HVec3 HVec3::closest_straight_line() const {
    HVec3 result = *this;
    HVec3 absolute = HVec3{
        std::abs(result.x),
        std::abs(result.y),
        std::abs(result.z)
    };

    if (std::min(std::min(absolute.x, absolute.y), absolute.z) == 0)
        return result;

    if (absolute.x < absolute.y &&
        absolute.x < absolute.z) {
        result.x = 0;
    } else if (absolute.y < absolute.z) {
        result.y = 0;
        absolute.y = 0;
    } else {
        result.z = 0;
        absolute.z = 0;
    }

    int32_t median;
    if (result.x == 0) {
        if (absolute.z < absolute.y)
            median = result.z;
        else
            median = result.y;
    } else if (result.y == 0) {
        if (absolute.x < absolute.z)
            median = result.x;
        else
            median = result.z;
    } else {
        if (absolute.x < absolute.y)
            median = result.x;
        else
            median = result.y;
    }

    if (absolute.x > absolute.y &&
        absolute.x > absolute.z) {
        result.x = -median;
    } else if (absolute.y > absolute.z) {
        result.y = -median;
    } else {
        result.z = -median;
    }

    return result;
}

Yngine::Direction HVec3::direction_to(HVec3 to) const {
    assert(*this != to);

    auto unit_diff = to - *this;
    unit_diff /= unit_diff.length();

    assert(unit_diff.length() == 1);

    const auto unit_diff_2 = HVec2{unit_diff};

    if (unit_diff_2 == HVec2{1, 0}) {
        return Yngine::Direction::SE;
    } else if (unit_diff_2 == HVec2{0, 1}) {
        return Yngine::Direction::NE;
    } else if (unit_diff_2 == HVec2{-1, 1}) {
        return Yngine::Direction::N;
    } else if (unit_diff_2 == HVec2{-1, 0}) {
        return Yngine::Direction::NW;
    } else if (unit_diff_2 == HVec2{0, -1}) {
        return Yngine::Direction::SW;
    } else if (unit_diff_2 == HVec2{1, -1}) {
        return Yngine::Direction::S;
    }

    abort();
}
