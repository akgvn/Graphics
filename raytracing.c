#include "geometry.h"
#include <stddef.h>
#include <stdio.h>   // printf
#include <stdlib.h>  // malloc, free
#include <float.h>   // maximum float value (FLT_MAX)
#include <math.h>    // M_PI, pow
#include <stdbool.h> // bool, true, false

#define WIDTH 1024
#define HEIGHT 768
#define FOV (M_PI/2.0)

// NOTE: Maybe move these structs to a Raytracing.h?
typedef struct Material
{
    Vec2f albedo;
    Vec3f diffuse_color;
    float specular_exponent; // "shininess"?
} Material;

typedef struct Sphere
{
    Vec3f center;
    float radius;
    Material material;
} Sphere;

typedef struct Ray
{
    Vec3f origin;
    Vec3f direction;
} Ray;

typedef struct Light
{
    Vec3f position;
    float intensity;
} Light;


// Returns true if the ray intersects the sphere. 
// Also mutates the first_intersect_distance parameter to reflect the location of the first intersection.
static bool
ray_intersects_sphere(const Ray* const ray, const Sphere* const sphere, float *first_intersect_distance) {
    // The fact that this function is not self-explanatory saddens me. Especially since I'm
    // trying to name things in an explanatory way. I'll put a list of resources to
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

Vec3f
reflection_vector(Vec3f light_direction, Vec3f surface_normal) {
    return sub_vec3f(
        light_direction,
        multiply_vec3f_with_scalar(surface_normal, 2.0 * multiply_vec3f(light_direction, surface_normal))
    );
}


static bool
scene_intersect(const Ray* ray, const Sphere* spheres, size_t number_of_spheres, Vec3f* hit_point, Vec3f* surface_normal, Material* material) {
    float spheres_distance = FLT_MAX;

    for (size_t i = 0; i < number_of_spheres; i++) {
        float distance_of_i;
        bool current_sphere_intersects = ray_intersects_sphere(ray, &spheres[i], &distance_of_i);

        // Finds the closest sphere.
        if (current_sphere_intersects && (distance_of_i < spheres_distance)) {
            spheres_distance = distance_of_i;

            *hit_point = add_vec3f(ray->origin, multiply_vec3f_with_scalar(ray->direction, distance_of_i));
            
            *surface_normal = sub_vec3f(*hit_point, spheres[i].center);
            vec3f_normalize(surface_normal);

            *material = spheres[i].material;
        }
    }

    return (spheres_distance < 1000);
}

// Return color of the sphere if intersected, otherwise returns background color.
static Vec3f
cast_ray(const Ray* const ray, const Sphere* const spheres, size_t number_of_spheres, 
         const Light* const lights, size_t number_of_lights) {
    Vec3f point, surface_normal;
    Material material;

    if (scene_intersect(ray, spheres, number_of_spheres, &point, &surface_normal, &material)) {
        float diffuse_light_intensity = 0, specular_light_intensity = 0;

        size_t count = 0;
        for (size_t i = 0; i < number_of_lights; i++) {
            Vec3f light_direction = sub_vec3f(lights[i].position, point);
            vec3f_normalize(&light_direction);

            // Shadows --------

            float light_distance = vec3f_norm(sub_vec3f(lights[i].position, point));

            Vec3f shadow_origin;
            Vec3f normal_divided = multiply_vec3f_with_scalar(surface_normal, 0.001);
            // Check if the point is in the shadow of lights[i].
            if (multiply_vec3f(light_direction, surface_normal) < 0) {
                shadow_origin = sub_vec3f(point, normal_divided);
            } else {
                shadow_origin = add_vec3f(point, normal_divided);
            }
            const Ray shadow_ray = {shadow_origin, light_direction};

            Vec3f shadow_point, shadow_normal; 
            Material temp_material;

            bool scene_thing = scene_intersect(&shadow_ray, spheres, number_of_spheres, &shadow_point, &shadow_normal, &temp_material);
            bool shadow_thing = (vec3f_norm(sub_vec3f(shadow_point, shadow_origin)) < light_distance);
            bool in_shadow = scene_thing && shadow_thing;

            if (in_shadow) { 
                continue;
            }

            // Shadows end ----

            // Lighting stuff starts here:
            float surface_illumination_intensity = multiply_vec3f(light_direction, surface_normal);
            if (surface_illumination_intensity < 0) surface_illumination_intensity = 0;

            // specular_illumination_intensity might not be the best name for this variable.
            float specular_illumination_intensity = multiply_vec3f(reflection_vector(light_direction, surface_normal), ray->direction);
            if (specular_illumination_intensity < 0) specular_illumination_intensity = 0;
            specular_illumination_intensity = pow(specular_illumination_intensity, material.specular_exponent);

            diffuse_light_intensity  += lights[i].intensity * surface_illumination_intensity;
            specular_light_intensity += lights[i].intensity * specular_illumination_intensity;
        }

        return add_vec3f(
            multiply_vec3f_with_scalar(material.diffuse_color, diffuse_light_intensity * material.albedo.x), 
            multiply_vec3f_with_scalar((Vec3f){1.0, 1.0, 1.0}, specular_light_intensity * material.albedo.y)
        );
    }

    return (Vec3f) {0.2, 0.7, 0.8}; // Background color
}

// NOTE: Move this to a utils.h (?) when working on Raycasting or Software Rendering.
void
dump_ppm_image(Vec3f* buffer, size_t width, size_t height) {
    // Size of buffer parametre must be width * height!
    // Dump the image to a PPM file.
    FILE *fp = fopen("out.ppm", "wb");
    char header[64];
    int count = sprintf(header, "P6\n%d %d\n255\n", width, height);
    fwrite(header, sizeof(char), count, fp); // Write the PPM header.
    
    for (size_t pixel = 0; pixel < width * height; pixel++) {
        
        {
            // Check if any of the vec elements is greater than one.
            Vec3f* vec = &buffer[pixel];
            float max = vec->x > vec->y ? vec->x : vec->y;
            max = max > vec->z ? max : vec->z;

            if (max > 1) {
                vec->x /= max;
                vec->y /= max;
                vec->z /= max;
            }
        }

        char rgb[3] = {
            (char)(buffer[pixel].x * 255), // NOTE: Could overflow without the check above!
            (char)(buffer[pixel].y * 255), // NOTE: Could overflow without the check above!
            (char)(buffer[pixel].z * 255), // NOTE: Could overflow without the check above!
        };
        // Note to self: fwrite moves the file cursor,
        // no need to use fseek or something.
        fwrite(rgb, sizeof(char), 3, fp);
    }

    fclose(fp);
}

static void
render(const Sphere* const spheres, size_t number_of_spheres, const Light* const lights, size_t number_of_lights) {
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
            size_t cell_index = col + row * WIDTH;
            framebuffer[cell_index] = cast_ray(&ray, spheres, number_of_spheres, lights, number_of_lights);
        }
    }
    dump_ppm_image(framebuffer, WIDTH, HEIGHT);
    free(framebuffer);
}

void
raytracing_main() {
    Material      ivory = {{0.6,  0.3}, {0.4, 0.4, 0.3},   50.0};
    Material red_rubber = {{0.9,  0.1}, {0.3, 0.1, 0.1},   10.0};

    Sphere spheres[] = {
        {{-3,    0,   -16}, 2,      ivory},
        {{-1.0, -1.5, -12}, 2, red_rubber},
        {{ 1.5, -0.5, -18}, 3, red_rubber},
        {{ 7,    5,   -18}, 4,      ivory},
    };

    Light lights[] = {
        {{-20, 20,  20}, 1.5},
        {{ 30, 50, -25}, 1.8},
        {{ 30, 20,  30}, 1.7},
    };

    render(spheres, 4, lights, 3);
}
