#pragma once

#include "Vec3.h"

struct Ray {
    Ray(const Vec3 &o, const Vec3 &d) : o(o), d(d) { }

    Vec3 pointAt(const float t) {
        return o + t * d;
    }

    Vec3 o;
    Vec3 d;
};
