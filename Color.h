#pragma once

#include "BasicTypes.h"

float max(const float a, const float b) {
    return a > b ? a : b;
}

float min(const float a, const float b) {
    return a < b ? a : b;
}

struct Color {
    // Names mostly from CSS colors
    static const Color BLACK;
    static const Color WHITE;
    static const Color RED;
    static const Color LIME;
    static const Color GREEN;
    static const Color DARK_GREEN;
    static const Color BLUE;
    static const Color DARK_SLATE_GRAY;
    static const Color ALMOST_BLACK;

    Color(const float a, const float r, const float g, const float b) : a(a), r(r), g(g), b(b) {}

    Color &operator+=(const Color &rhs) {
        a += rhs.a;
        r += rhs.r;
        g += rhs.g;
        b += rhs.b;
        return *this;
    }

    Color &operator*=(const float s) {
        a *= s;
        r *= s;
        g *= s;
        b *= s;
        return *this;
    }

    Color operator+(const Color &rhs) const {
        return Color(a + rhs.a, r + rhs.r, g + rhs.g, b + rhs.b);
    }

    Color operator*(const Color &rhs) const {
        return Color(a * rhs.a, r * rhs.r, g * rhs.g, b * rhs.b);
    }

    Color operator*(const float s) const {
        return Color(a * s, r * s, g * s, b * s);
    }

    void saturate() {
        a = max(0.0, min(1.0, a));
        r = max(0.0, min(1.0, r));
        g = max(0.0, min(1.0, g));
        b = max(0.0, min(1.0, b));
    }

    u32 toU32() const {
        return (((u32)(255 * a) << 24) | ((u32)(255 * r) << 16) | ((u32)(255 * g) << 8) | (u32)(255 * b));
    }

    float a;
    float r;
    float g;
    float b;
};

const Color Color::BLACK      = Color(1.0, 0.0, 0.0, 0.0);
const Color Color::WHITE      = Color(1.0, 1.0, 1.0, 1.0);
const Color Color::RED        = Color(1.0, 1.0, 0.0, 0.0);
const Color Color::LIME       = Color(1.0, 0.0, 1.0, 0.0);
const Color Color::GREEN      = Color(1.0, 0.0, 0.5, 0.0);
const Color Color::DARK_GREEN = Color(1.0, 0.0, 100.0 / 255.0, 0.0);
const Color Color::BLUE       = Color(1.0, 0.0, 0.0, 1.0);

const Color Color::DARK_SLATE_GRAY = Color(1.0, 47.0 / 255.0, 79.0 / 255.0, 79.0 / 255.0);
const Color Color::ALMOST_BLACK    = Color(1.0, 0.1, 0.1, 0.1);
