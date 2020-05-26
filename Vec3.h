#pragma once

#include "BasicTypes.h"

struct Vec3 {
    static const Vec3 ZERO;
    static const Vec3 Y;

    Vec3(float x, float y, float z) : x(x), y(y), z(z) { }

    void normalize() {
        float invLen = 1.0 / length();
        if (invLen > 0) {
            x *= invLen;
            y *= invLen;
            z *= invLen;
        }
    }

    float length() const {
        return sqrtf(x * x + y * y + z * z);
    }

    float dot(const Vec3 &v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    Vec3 cross(const Vec3 &rhs) const {
        return Vec3(
            y * rhs.z - z * rhs.y, 
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        );
    }

    Vec3 operator+(const Vec3 &b) const {
        return Vec3(x + b.x, y + b.y, z + b.z);
    }

    Vec3 operator-(const Vec3 &b) const {
        return Vec3(x - b.x, y - b.y, z - b.z);
    }

    float x;
    float y;
    float z;
};

Vec3 operator*(const float s, const Vec3 &rhs) {
    return Vec3(rhs.x * s, rhs.y * s, rhs.z * s);
}

Vec3 normalize(const Vec3 &v) {
    float invLen = 1.0 / v.length();
    if (invLen > 0) {
        return invLen * v;
    } else {
        return v;
    }
}

const Vec3 Vec3::ZERO = Vec3(0, 0, 0);
const Vec3 Vec3::Y    = Vec3(0, 1, 0);
