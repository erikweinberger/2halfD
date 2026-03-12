#ifndef MATH_TYPES_H
#define MATH_TYPES_H

#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <iostream>
#include <numbers>
#include <vector>

namespace TwoHalfD {

struct XYVector {
    int x, y;

    XYVector operator-(const XYVector &other) {
        XYVector result{this->x - other.x, this->y - other.y};
        return result;
    }

    XYVector operator+(const XYVector &other) {
        XYVector result{this->x + other.x, this->y + other.y};
        return result;
    }

    bool operator==(const XYVector &other) const {
        return this->x == other.x && this->y == other.y;
    }

    friend std::ostream &operator<<(std::ostream &os, const XYVector &v) {
        os << "(" << v.x << " , " << v.y << ")";
        return os;
    }
};

struct XYVectorf {
    float x, y;

    XYVectorf() : x(0), y(0) {}

    XYVectorf(float x, float y) : x(x), y(y) {}
    XYVectorf(const sf::Vector2f &v) : x(v.x), y(v.y) {}

    float length() const {
        return std::sqrt(x * x + y * y);
    }

    float lengthSquared() const {
        return x * x + y * y;
    }

    XYVectorf normalized() const {
        float len = length();
        if (len > 0) {
            return XYVectorf(x / len, y / len);
        }
        return XYVectorf(0, 0);
    }

    XYVectorf operator-(const XYVectorf &other) const {
        XYVectorf result{this->x - other.x, this->y - other.y};
        return result;
    }

    XYVectorf operator+(const XYVectorf &other) const {
        XYVectorf result{this->x + other.x, this->y + other.y};
        return result;
    }

    XYVectorf operator*(const float scalar) const {
        return XYVectorf(this->x * scalar, this->y * scalar);
    }

    friend XYVectorf operator*(const float scalar, const XYVectorf &vec) {
        return XYVectorf(vec.x * scalar, vec.y * scalar);
    }

    bool operator==(const XYVectorf &other) const {
        return this->x == other.x && this->y == other.y;
    }

    friend std::ostream &operator<<(std::ostream &os, const XYVectorf &v) {
        os << "(" << v.x << " , " << v.y << ")";
        return os;
    }
};

using Polygon = std::vector<TwoHalfD::XYVectorf>;

inline float dot(const XYVectorf &a, const XYVectorf &b) {
    return a.x * b.x + a.y * b.y;
}

struct Position {

    union {
        XYVectorf pos;
        sf::Vector2f posf;
    };
    float direction = 0.f;

    Position(float x = 2 * 256.0f, float y = 3 * 256.0f, float dir = (3 * std::numbers::pi_v<float> / 2.0f)) : pos{x, y}, direction(dir) {}

    Position(sf::Vector2f v, float dir = 0.f) : posf(v), direction(dir) {}

    Position(XYVectorf v, float dir = 0.f) : pos(v), direction(dir) {}

    Position operator+(const Position &other) const {
        return Position(pos.x + other.pos.x, pos.y + other.pos.x, direction + other.direction);
    }

    Position &operator+=(const Position &other) {
        pos.x += other.pos.x;
        pos.y += other.pos.y;
        direction += other.direction;
        return *this;
    }

    Position operator-(const Position &other) const {
        return Position(pos.x - other.pos.x, pos.y - other.pos.x, direction - other.direction);
    }

    Position &operator-=(const Position &other) {
        pos.x -= other.pos.x;
        pos.y -= other.pos.y;
        direction -= other.direction;
        return *this;
    }
};

} // namespace TwoHalfD

#endif
