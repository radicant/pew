#pragma once

#include "Ray.h"
#include "Vec3.h"

struct Matrix {
    
    Ray operator*(const Ray &rhs) const {
        return Ray(
            this->multPoint(rhs.o),
            this->multVector(rhs.d)
        );
    }

    Vec3 multPoint(const Vec3 &rhs) const {
        // w is 1
        float x = m[0][0] * rhs.x + m[0][1] * rhs.y + m[0][2] * rhs.z + m[0][3];
        float y = m[1][0] * rhs.x + m[1][1] * rhs.y + m[1][2] * rhs.z + m[1][3];
        float z = m[2][0] * rhs.x + m[2][1] * rhs.y + m[2][2] * rhs.z + m[2][3];
        return Vec3(x, y, z);
    }

    Vec3 multVector(const Vec3 &rhs) const {
        // w is 0
        float x = m[0][0] * rhs.x + m[0][1] * rhs.y + m[0][2] * rhs.z;
        float y = m[1][0] * rhs.x + m[1][1] * rhs.y + m[1][2] * rhs.z;
        float z = m[2][0] * rhs.x + m[2][1] * rhs.y + m[2][2] * rhs.z;
        return Vec3(x, y, z);
    }
    
    static Matrix lookAt(const Vec3 &eye, const Vec3 &at) {
        Vec3 look = normalize(at - eye);
        Vec3 right = look.cross(Vec3::Y);
        Vec3 up = right.cross(look);

        Matrix camToWorld;

        camToWorld.m[0][0] = right.x;
        camToWorld.m[0][1] = right.y;
        camToWorld.m[0][2] = right.z;
        camToWorld.m[0][3] = -right.dot(eye);
                  
        camToWorld.m[1][0] = up.x;
        camToWorld.m[1][1] = up.y;
        camToWorld.m[1][2] = up.z;
        camToWorld.m[1][3] = -up.dot(eye);
                  
        camToWorld.m[2][0] = -look.x;
        camToWorld.m[2][1] = -look.y;
        camToWorld.m[2][2] = -look.z;
        camToWorld.m[2][3] = -look.dot(eye);

        return camToWorld;
    }

    float m[3][4];
};