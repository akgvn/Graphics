#include "geometry.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 1024
#define HEIGHT 768

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


bool sphere_ray_intersect(const Sphere* const sphere, const Ray* const ray, float t0) {
    Vec3f L = sphere->center - origin; // TODO Use the geometry.h function!
}

void render() {
    Vec3f *framebuffer = malloc(WIDTH*HEIGHT*sizeof(Vec3f));

    // Each pixel in the resulting image will have an RGB value, represented by the Vec3f type.
    for (size_t row = 0; row < HEIGHT; row++) {
        for (size_t col = 0; col < WIDTH; col++) {
            Vec3f cell = {
                .x = (float)row /(float)HEIGHT,
                .y = (float)col /(float)WIDTH,
                .z = 0
            };
            // Writing [col * row + WIDTH] instead of the current expression just cost 
            // me 1.5 hours of debugging. Sigh. Don't write code when you're sleepy!
            framebuffer[col + row * WIDTH] = cell;
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
    render();
    return 0;
}