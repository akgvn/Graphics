#include "geometry.h"
#include <stddef.h>
#include <stdio.h>   // printf
#include <stdlib.h>  // malloc, free
#include <float.h>   // maximum float value (FLT_MAX)

#define _USE_MATH_DEFINES // To make sure M_PI is defined -- zig cc wants this.
#include <math.h>    // M_PI, pow

#include <stdbool.h> // bool, true, false

#define WIDTH 1024
#define HEIGHT 768
#define FOV (M_PI/2.0)

// NOTE: Maybe move these structs to a Raytracing.h?
typedef struct Material
{
    float refractive_index;
    Vec4f albedo;
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

    // If looking at earlier commits, you might see that I thought the compiler optimizes out some of the next 3 lines.
    // Actually it doesn't, I was comparing a float* to 0. So the compiler thought, rightly, we'd never execute the
    // if (pointer is lower than 0) { do thing } instruction. There should really be a warning for this, and if there is one,
    // it should really be enabled by default.
    if (*first_intersect_distance < 0.0) { *first_intersect_distance = last_intersect_distance; } // Maybe intersects at only one point?
    if (  last_intersect_distance < 0.0) { return false; }
    else { return true; }
}

Vec3f
reflection_vector(Vec3f light_direction, Vec3f surface_normal) {
    return sub_vec3f(
        light_direction,
        multiply_vec3f_with_scalar(surface_normal, 2.0 * multiply_vec3f(light_direction, surface_normal))
    );
}

Vec3f refraction_vector(Vec3f light_vector, Vec3f normal, float refractive_index) { // Snell's law
    float cos_incidence = -1 * multiply_vec3f(light_vector, normal); // Cosine of the angle of the incidence

    if      (cos_incidence >  1) { cos_incidence =  1; }
    else if (cos_incidence < -1) { cos_incidence = -1; }

    float refractive_indices_ratio; // n1 / n2, n1 is refractive index of outside, n2 is inside object
    Vec3f refraction_normal;

    if (cos_incidence < 0) { // Is the ray inside the object?
        cos_incidence = -cos_incidence;
        refractive_indices_ratio = refractive_index; // swap the indices
        refraction_normal = multiply_vec3f_with_scalar(normal, -1); // invert the normal
    } else { // not inside the object, go on
        refractive_indices_ratio = 1.0 / refractive_index;
        refraction_normal = normal;
    }

    float cos_refraction_squared = 1 - ((refractive_indices_ratio * refractive_indices_ratio) * (1 - cos_incidence*cos_incidence));
    if (cos_refraction_squared < 0) {
        return (Vec3f) {0, 0, 0};
    } else {
        return add_vec3f(
            multiply_vec3f_with_scalar(light_vector, refractive_indices_ratio),
            multiply_vec3f_with_scalar(refraction_normal, (refractive_indices_ratio * cos_incidence) - sqrtf(cos_refraction_squared))
        );
    }
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
         const Light* const lights, size_t number_of_lights, size_t depth) {
    Vec3f point, surface_normal;
    Material material;

    if (depth > 5 || !scene_intersect(ray, spheres, number_of_spheres, &point, &surface_normal, &material)) {
        return (Vec3f) {0.2, 0.7, 0.8}; // Background color
    }

    Vec3f reflect_color;
    {   // Reflection stuff happens in this scope.
        Vec3f reflect_direction = reflection_vector(ray->direction, surface_normal); vec3f_normalize(&reflect_direction);
    
        Vec3f reflect_origin; // offset the original point to avoid occlusion by the object itself
        {
            if (multiply_vec3f(reflect_direction, surface_normal) < 0) {
                reflect_origin = sub_vec3f(point, multiply_vec3f_with_scalar(surface_normal, 1e-3));
            } else {
                reflect_origin = add_vec3f(point, multiply_vec3f_with_scalar(surface_normal, 1e-3));
            }
        }

        Ray reflection_ray  = { reflect_origin, reflect_direction };
        reflect_color = cast_ray(&reflection_ray, spheres, number_of_spheres, lights, number_of_lights, depth + 1);
    }

    Vec3f refract_color;
    {   // refraction stuff happens in this scope.
        Vec3f refract_direction = refraction_vector(ray->direction, surface_normal, material.refractive_index); vec3f_normalize(&refract_direction);
    
        Vec3f refract_origin;
        {
            if (multiply_vec3f(refract_direction, surface_normal) < 0) {
                refract_origin = sub_vec3f(point, multiply_vec3f_with_scalar(surface_normal, 1e-3));
            } else {
                refract_origin = add_vec3f(point, multiply_vec3f_with_scalar(surface_normal, 1e-3));
            }
        }

        Ray refraction_ray  = { refract_origin, refract_direction };
        refract_color = cast_ray(&refraction_ray, spheres, number_of_spheres, lights, number_of_lights, depth + 1);
    }

    float diffuse_light_intensity = 0, specular_light_intensity = 0;
    for (size_t i = 0; i < number_of_lights; i++) {
            Vec3f light_direction = sub_vec3f(lights[i].position, point);
            vec3f_normalize(&light_direction);

            bool in_shadow = false;
            {
                // Can this point see the current light?
                Vec3f light_to_point_vec = sub_vec3f(lights[i].position, point);
                float light_distance = vec3f_norm(light_to_point_vec);

                Vec3f shadow_origin;
                if (multiply_vec3f(light_direction, surface_normal) < 0) {
                    shadow_origin = sub_vec3f(point, multiply_vec3f_with_scalar(surface_normal, 1e-3));
                } else {
                    shadow_origin = add_vec3f(point, multiply_vec3f_with_scalar(surface_normal, 1e-3));
                }

                Vec3f shadow_point, shadow_normal; Material temp_material;

                Ray temp_ray = {shadow_origin, light_direction};
                bool light_intersected = scene_intersect(&temp_ray, spheres, number_of_spheres, &shadow_point, &shadow_normal, &temp_material);
                Vec3f pt_to_origin = sub_vec3f(shadow_point, shadow_origin);
                bool obstruction_closer_than_light = vec3f_norm(pt_to_origin) < light_distance;
                in_shadow = light_intersected && obstruction_closer_than_light;
            }
            if (in_shadow) continue;

            // Diffuse Lighting:
            float surface_illumination_intensity = multiply_vec3f(light_direction, surface_normal);
            if (surface_illumination_intensity < 0) surface_illumination_intensity = 0;
            diffuse_light_intensity  += lights[i].intensity * surface_illumination_intensity;

            // Specular Lighting:
            float specular_illumination_intensity = multiply_vec3f(reflection_vector(light_direction, surface_normal), ray->direction);
            if (specular_illumination_intensity < 0) specular_illumination_intensity = 0;
            specular_illumination_intensity = pow(specular_illumination_intensity, material.specular_exponent);
            specular_light_intensity += lights[i].intensity * specular_illumination_intensity;
    }

    Vec3f lighting = add_vec3f(
        multiply_vec3f_with_scalar(material.diffuse_color, diffuse_light_intensity * material.albedo.x), 
        multiply_vec3f_with_scalar((Vec3f){1.0, 1.0, 1.0}, specular_light_intensity * material.albedo.y)
    );

    // if (material.albedo.w > 0.1) print_vec4f(material.albedo);
    Vec3f reflect_refract = add_vec3f(
        multiply_vec3f_with_scalar(reflect_color, material.albedo.z),
        multiply_vec3f_with_scalar(refract_color, material.albedo.w)
    );
    
    return add_vec3f(
        lighting,
        reflect_refract
    );
}

// NOTE: Move this to a utils.h (?) when working on Raycasting or Software Rendering.
void
dump_ppm_image(Vec3f* buffer, size_t width, size_t height) {
    // Size of buffer parametre must be width * height!
    // Dump the image to a PPM file.
    
    FILE *fp; 

    // zig cc (clang) wanted me to use fopen_s, but has a problem with sprintf. I don't know what's up, GCC works.
    if (fopen_s(&fp, "out.ppm", "wb") != 0) {
        puts("Can't open file for writing.");
        return;
    }

    char header[64];
    size_t count = sprintf(header, "P6\n%d %d\n255\n", width, height);
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
        }; // TODO: We crash after reaching this line when compiled with zig cc.

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
            framebuffer[col + row * WIDTH] = cast_ray(&ray, spheres, number_of_spheres, lights, number_of_lights, 0);
        }
    }
    dump_ppm_image(framebuffer, WIDTH, HEIGHT);
    free(framebuffer);
}

void
raytracing_main() {
    Material      ivory = {1.0, {0.6,  0.3, 0.1, 0.0}, {0.4, 0.4, 0.3},   50.0};
    Material      glass = {1.5, {0.0,  0.5, 0.1, 0.8}, {0.6, 0.7, 0.8},  125.0};
    Material red_rubber = {1.0, {0.9,  0.1, 0.0, 0.0}, {0.3, 0.1, 0.1},   10.0};
    Material     mirror = {1.0, {0.0, 10.0, 0.8, 0.0}, {1.0, 1.0, 1.0}, 1425.0};

    Sphere spheres[] = {
        {{-3,    0,   -16}, 2,      ivory},
        {{-1.0, -1.5, -12}, 2,      glass},
        {{ 1.5, -0.5, -18}, 3, red_rubber},
        {{ 7,    5,   -18}, 4,     mirror},
    };

    Light lights[] = {
        {{-20, 20,  20}, 1.5},
        {{ 30, 50, -25}, 1.8},
        {{ 30, 20,  30}, 1.7},
    };

    render(spheres, 4, lights, 3);
}
