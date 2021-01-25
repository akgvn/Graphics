#include "geometry.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h> // for maximum float value, FLT_MAX
#include <math.h> // for M_PI
#include <stdbool.h>

#define WIDTH 1024
#define HEIGHT 768
#define FOV (M_PI/2.0)

// #define MIN(a,b) (((a)<(b))?(a):(b))
// #define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct
{
    Vec3f center;
    float radius;
} Sphere;

typedef struct
{
    Vec3f origin;
    Vec3f direction;
} Ray;


// Returns true if the ray intersects the sphere. 
// Also mutates the t0 parameter to reflect the location of the first intersection.
bool ray_intersects_sphere(const Ray* const ray, const Sphere* const sphere, float *t0) {
    // The fact that this function is not self-explanatory saddens me. Especially since I'm
    // trying to name things in a explanatory way. I'll put a list of resources to
    // understand what is going on here to the readme.

    Vec3f L = sub_vec3f(sphere->center, ray->origin); // sphere_center_to_ray_origin_distance

    float tc = multiply_vec3f(L, ray->direction); // tc is the distance of sphere center to ray origin along the ray direction vector.

    float center_to_ray_straight_distance = multiply_vec3f(L, L) - tc*tc;
    float radius_squared = sphere->radius * sphere->radius;

    // Check if the ray is not inside the sphere
    if (center_to_ray_straight_distance > radius_squared) return false;

    float half_length_of_ray_inside_circle = sqrtf(radius_squared - center_to_ray_straight_distance);

    *t0 = tc - half_length_of_ray_inside_circle;
    float t1 = tc + half_length_of_ray_inside_circle;

    if (t0 < 0) *t0 = t1; // Maybe intersects at only one point?
    if (t0 < 0) return false;

    return true;
}

// Return color of the sphere if intersected, otherwise returns background color.
Vec3f cast_ray(const Ray* const ray, const Sphere* const sphere) {
    float max_possible_distance = FLT_MAX;
    Vec3f ret = {0.2, 0.7, 0.8}; // Background color
    if (ray_intersects_sphere(ray, sphere, &max_possible_distance)) {
        // Change to sphere color since there is an intersection.
        ret = (Vec3f) {0.4, 0.4, 0.3};
    }
    return ret;
}

void render(const Sphere* const sphere) {
    Vec3f *framebuffer = malloc(WIDTH*HEIGHT*sizeof(Vec3f));

    // Each pixel in the resulting image will have an RGB value, represented by the Vec3f type.
    for (size_t row = 0; row < HEIGHT; row++) {
        for (size_t col = 0; col < WIDTH; col++) {
            // Sweeping the field of view with rays.
            float screen_width = 2 * tan(FOV/2.);
            float x =  (screen_width * (col + 0.5)/(float)WIDTH  - 1)* WIDTH/(float)HEIGHT;
            float y = -(screen_width * (row + 0.5)/(float)HEIGHT - 1);
            Vec3f dir = {x, y, -1};
            vec3f_normalize(&dir);
            Ray ray = {{0, 0, 0}, dir};

            // Writing [col * row + WIDTH] instead of the current expression just cost 
            // me 1.5 hours of debugging. Sigh. Don't write code when you're sleepy!
            framebuffer[col + row * WIDTH] = cast_ray(&ray, sphere);
        }
    }

    // Dump the image to a PPM file.
    FILE *fp = fopen("out.ppm", "wb");
    char header[64];
    int count = sprintf(header, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    fwrite(header, sizeof(char), count, fp); // Write the PPM header.
    
    for (size_t pixel = 0; pixel < WIDTH * HEIGHT; pixel++) {
        char rgb[3] = {
            (char)(framebuffer[pixel].x * 255),
            (char)(framebuffer[pixel].y * 255),
            (char)(framebuffer[pixel].z * 255),
        };
        // Note to self: fwrite moves the file cursor,
        // no need to use fseek or something.
        fwrite(rgb, sizeof(char), 3, fp);
    }

    fclose(fp); free(framebuffer);
}

int main(int argc, char const *argv[])
{
    Vec3f sphere_center = {-3, 0, -16};

    Sphere sphere = {sphere_center, 2};
    render(&sphere);
    return 0;
}