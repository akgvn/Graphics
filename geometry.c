// Notes:
// Vec3i and its functions are not implemented.
// Code is ported to C from C++ templates at https://github.com/ssloy/tinyraytracer/blob/master/geometry.h

// The lines of code could have been fewer if I used macros but I'm avoiding macros for now, I'd rather not deal with them for the time being.

#include "geometry.h"
#include <math.h>
#include <stdio.h>

// Vec3f -------------------------------------

float vec3f_norm(Vec3f vec) { return sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z); }

void vec3f_normalize(Vec3f* const vec) {
    float norm = vec3f_norm(*vec);
    vec->x /= norm; vec->y /= norm; vec->z /= norm;
}

Vec3f cross(Vec3f v1, Vec3f v2) {
    Vec3f ret = {
        .x = v1.y*v2.z - v1.z*v2.y,
        .y = v1.z*v2.x - v1.x*v2.z,
        .z = v1.x*v2.y - v1.y*v2.x,
    };
    return ret;
}

float multiply_vec3f(Vec3f lhs, Vec3f rhs) { return (lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z); }

Vec3f multiply_vec3f_with_scalar(Vec3f lhs, float rhs) {
    Vec3f ret = {
        .x = lhs.x * rhs,
        .y = lhs.y * rhs,
        .z = lhs.z * rhs,
    };
    return ret;
}

Vec3f add_vec3f(Vec3f lhs, Vec3f rhs) {
    Vec3f ret = {
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y,
        .z = lhs.z + rhs.z,
    };
    return ret;
}

Vec3f sub_vec3f(Vec3f lhs, Vec3f rhs) {
    Vec3f ret = {
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y,
        .z = lhs.z - rhs.z,
    };
    return ret;
}

// Vec4f -------------------------------------

float multiply_vec4f(Vec4f lhs, Vec4f rhs) {
    return (lhs.x * rhs.x + 
            lhs.y * rhs.y + 
            lhs.z * rhs.z +
            lhs.w * rhs.w);
}

Vec4f multiply_vec4f_with_scalar(Vec4f lhs, float rhs) {
    Vec4f ret = {
        .x = lhs.x * rhs,
        .y = lhs.y * rhs,
        .z = lhs.z * rhs,
        .w = lhs.w * rhs,
    };
    return ret;
}

Vec4f add_vec4f(Vec4f lhs, Vec4f rhs) {
    Vec4f ret = {
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y,
        .z = lhs.z + rhs.z,
        .w = lhs.w + rhs.w,
    };
    return ret;
}

Vec4f sub_vec4f(Vec4f lhs, Vec4f rhs) {
    Vec4f ret = {
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y,
        .z = lhs.z - rhs.z,
        .w = lhs.w - rhs.w,
    };
    return ret;
}

// Print ----------------------------------

void print_vec4f(Vec4f vec) { printf("Vec4f { %f, %f, %f, %f }\n", vec.x, vec.y, vec.z, vec.w); }

void print_vec3f(Vec3f vec) { printf("Vec3f { %f, %f, %f }\n", vec.x, vec.y, vec.z); }

void print_vec2f(Vec2f vec) { printf("Vec2f { %f, %f }\n", vec.x, vec.y); }