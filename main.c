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
    Vec3f diffuse_color;
} Material;

typedef struct
{
    Vec3f center;
    float radius;
    Material material;
} Sphere;

typedef struct
{
    Vec3f origin;
    Vec3f direction;
} Ray;


// Returns true if the ray intersects the sphere. 
// Also mutates the first_intersect_distance parameter to reflect the location of the first intersection.
bool ray_intersects_sphere(const Ray* const ray, const Sphere* const sphere, float *first_intersect_distance) {
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

    *first_intersect_distance     = tc - half_length_of_ray_inside_circle;
    float last_intersect_distance = tc + half_length_of_ray_inside_circle;

    if (first_intersect_distance < 0) *first_intersect_distance = last_intersect_distance; // Maybe intersects at only one point?
    if (first_intersect_distance < 0) return false;

    return true;
}

bool scene_intersect(const Ray* ray, const Sphere* spheres, size_t number_of_spheres, Vec3f* hit_point, Vec3f* N, Material* material) {
    float spheres_distance = FLT_MAX;

    for (size_t i = 0; i < number_of_spheres; i++) {
        float distance_of_i;
        bool current_sphere_intersects = ray_intersects_sphere(ray, &spheres[i], &distance_of_i);

        // Finds the closest sphere.
        if (current_sphere_intersects && (distance_of_i < spheres_distance)) {
            spheres_distance = distance_of_i;

            *hit_point = add_vec3f(ray->origin, multiply_vec3f_with_scalar(ray->direction, distance_of_i));
            
            *N = sub_vec3f(*hit_point, spheres[i].center);
            vec3f_normalize(N); // NOTE: As far as I can tell, N is the surface normal at the point where the cast ray hit.

            *material = spheres[i].material;
        }
    }

    return (spheres_distance < 1000);
}

// Return color of the sphere if intersected, otherwise returns background color.
Vec3f cast_ray(const Ray* const ray, const Sphere* const spheres, size_t number_of_spheres) {
    Vec3f point, N;
    Material material;
    // I feel like the locals I just declared will be part of a sphere in future. We'll see. 

    if (scene_intersect(ray, spheres, number_of_spheres, &point, &N, &material)) {
        return material.diffuse_color;
    }

    return (Vec3f) {0.2, 0.7, 0.8}; // Background color
}

void render(const Sphere* const spheres, size_t number_of_spheres) {
    Vec3f *framebuffer = malloc(WIDTH*HEIGHT*sizeof(Vec3f));

    // Each pixel in the resulting image will have an RGB value, represented by the Vec3f type.
    for (size_t row = 0; row < HEIGHT; row++) {
        for (size_t col = 0; col < WIDTH; col++) {
            // Sweeping the field of view with rays.

            const float camera_screen_distance = 1.0;
            float screen_width = 2 * tan(FOV/2.) * camera_screen_distance;
            float x =  (screen_width * (col + 0.5)/(float)WIDTH  - 1)* WIDTH/(float)HEIGHT;
            float y = -(screen_width * (row + 0.5)/(float)HEIGHT - 1);

            Vec3f dir = {x, y, -1};
            
            vec3f_normalize(&dir);
            Ray ray = {{0, 0, 0}, dir};

            // Writing [col * row + WIDTH] instead of the current expression just cost 
            // me 1.5 hours of debugging. Sigh. Don't write code when you're sleepy!
            framebuffer[col + row * WIDTH] = cast_ray(&ray, spheres, number_of_spheres);
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
    Material      ivory = {{0.4, 0.4, 0.3}};
    Material red_rubber = {{0.3, 0.1, 0.1}};

    Sphere spheres[] = {
        (Sphere) { (Vec3f) {-3,    0,   -16}, 2,      ivory},
        (Sphere) { (Vec3f) {-1.0, -1.5, -12}, 2, red_rubber},
        (Sphere) { (Vec3f) { 1.5, -0.5, -18}, 3, red_rubber},
        (Sphere) { (Vec3f) { 7,    5,   -18}, 4,      ivory},
    };

    render(spheres, 4);
    return 0;
}




    