#pragma once

#include <float.h>
#include <math.h>

#include "Color.h"
#include "Ray.h"
#include "Vec3.h"

struct Sphere {
    Sphere(const Vec3 &o, const float r, const Color &c) : o(o), r(r), c(c) {}

    float intersect(const Ray &r) const {
        Vec3 l = r.o - o;
        float a = r.d.dot(r.d);
        float b = 2 * r.d.dot(l);
        float c = l.dot(l) - this->r * this->r;
        return quadratic(a, b, c);
    }

    const Vec3 o;
    const float r;
    const Color c;

private: 
    static float quadratic(const float a, const float b, const float c) {
        float x = FLT_MAX;
        float disc = b * b - 4 * a * c;
        if (disc == 0) {
            x = -0.5 * b / a;
        }
        else if (disc > 0) {
            float q = (b > 0) ? -0.5 * (b - sqrt(disc)) : -0.5 * (b + sqrt(disc));
            float x0 = q / a;
            float x1 = c / q;
            if (x0 < x1 && x0 >= 0) {
                x = x0;
            }
            else if (x1 >= 0) {
                x = x1;
            }
        }
        return x;
    }
};
