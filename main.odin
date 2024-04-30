package graphics

import "core:fmt"
import "core:os"
import "core:io"
import "core:math"
import "core:math/linalg"

WIDTH  :: 1024
HEIGHT ::  768
FOV : f32 : (linalg.PI/2.0)
BACKGROUND_COLOR :: Vec3f {0.2, 0.7, 0.8}

Vec3f :: [3]f32
Vec4f :: [4]f32

Material :: struct {
    refractive_index:  f32,
    albedo:            Vec4f,
    diffuse_color:     Vec3f,
    specular_exponent: f32, // "shininess"?
}

DEFAULT_MATERIAL : Material : {
    refractive_index = 1.0,
    albedo = {1.0,  0.0, 0.0, 0.0},
    diffuse_color = {0.0, 0.0, 0.0},
    specular_exponent = 0.0
}
// Initialize all materials that have an effect on output to this default!

Sphere :: struct {
    center:   Vec3f,
    radius:   f32,
    material: Material,
}

Ray :: struct {
    origin:    Vec3f,
    direction: Vec3f,
}

Light :: struct {
    position:  Vec3f,
    intensity: f32,
}

dump_ppm_image :: proc(buffer: []Vec3f, width: int, height: int) {
    // Dump the image to a PPM file.

    assert(
        len(buffer) == width * height,
        "Buffer passed in to dump_ppm_image must have exactly width * height items!"
    )

    file_handle, file_open_error := os.open("out.ppm", os.O_CREATE | os.O_WRONLY)
    if file_open_error != 0 {
        fmt.println("Can't open file for writing: ", file_open_error)
        return
    }
    // No need to close the file handle since we're closing it through the stream below.

    stream := os.stream_from_handle(file_handle)
    defer {
        close_error := io.close(stream)
        if close_error != .None {
            fmt.println("Couldn't close stream, the error: ", close_error)
        }
    }

    header: [64]byte
    str: string = fmt.bprintf(header[:], "P6\n%d %d\n255\n", width, height)
    written_count, header_write_error := io.write(stream, header[:len(str)]) // Write the PPM header.
    if header_write_error != .None {
        fmt.println("Couldn't write PPM header, error: ", header_write_error)
        return
    }

    for pixel in buffer {
        // Check if any of the vec elements is greater than one.
        pixel := pixel
        if m := max(pixel.x, pixel.y, pixel.z); m > 1 {
            pixel /= m
        }

        rgb: [3]byte = {
            byte(pixel.x * 255), byte(pixel.y * 255), byte(pixel.z * 255),
        }

        written_count, color_write_error := io.write(stream, rgb[:])
        if color_write_error != .None {
            fmt.println("Error writing color at index ", pixel, " error: ", color_write_error)
        }
    }
}

// Returns true if the ray intersects the sphere. 
// Also mutates the first_intersect_distance parameter to reflect the location of the first intersection.
ray_intersects_sphere :: proc(ray: Ray, sphere: Sphere) -> (intersects: bool, first_intersect_distance: f32) {
    // The fact that this function is not self-explanatory saddens me. Especially since I'm
    // trying to name things in an explanatory way. I'll put a list of resources to
    // understand what is going on here to the readme.

    sphere_center_to_ray_origin_distance := sphere.center - ray.origin

    // tc is the distance of sphere center to ray origin along the ray direction vector.
    tc := linalg.dot(sphere_center_to_ray_origin_distance, ray.direction)

    center_to_ray_straight_distance := linalg.dot(
        sphere_center_to_ray_origin_distance,
        sphere_center_to_ray_origin_distance
    ) - tc*tc
    radius_squared := sphere.radius * sphere.radius

    // Check if the ray is not inside the sphere
    if (center_to_ray_straight_distance > radius_squared) { return false, 0.0 }

    half_length_of_ray_inside_circle := math.sqrt(radius_squared - center_to_ray_straight_distance)

    first_intersect_distance = tc - half_length_of_ray_inside_circle
    last_intersect_distance  := tc + half_length_of_ray_inside_circle

    if first_intersect_distance < 0.0 {
        // Maybe intersects at only one point?
        first_intersect_distance = last_intersect_distance
    }

    intersects = last_intersect_distance < 0.0 ? false : true
    return intersects, first_intersect_distance
}

reflection_vector :: proc(light_direction: Vec3f, surface_normal: Vec3f) -> Vec3f {
    return light_direction - (surface_normal * 2.0 * linalg.dot(light_direction, surface_normal))
}

// Snell's law
refraction_vector :: proc(light_vector: Vec3f, normal: Vec3f, refractive_index: f32) -> Vec3f
{
    cos_incidence := -1 * linalg.dot(light_vector, normal) // Cosine of the angle of the incidence

    cos_incidence = clamp(cos_incidence, -1, 1)

    refractive_indices_ratio: f32 // n1 / n2, n1 is refractive index of outside, n2 is inside object
    refraction_normal: Vec3f

    if cos_incidence < 0 { // Is the ray inside the object?
        cos_incidence = -cos_incidence
        refractive_indices_ratio = refractive_index // swap the indices
        refraction_normal = (normal * -1) // invert the normal
    } else { // not inside the object, go on
        refractive_indices_ratio = 1.0 / refractive_index
        refraction_normal = normal
    }

    cos_refraction_squared: f32 = 1 - (refractive_indices_ratio * refractive_indices_ratio) * (1 - cos_incidence*cos_incidence)
    if cos_refraction_squared < 0 {
        return {0, 0, 0}
    } else {
        return (light_vector * refractive_indices_ratio) + (refraction_normal * (refractive_indices_ratio * cos_incidence) - math.sqrt(cos_refraction_squared))
    }
}

scene_intersect :: proc(
    ray: Ray,
    spheres: []Sphere
) -> (intersects: bool, hit_point: Vec3f, surface_normal: Vec3f, material: Material) {
    material = DEFAULT_MATERIAL
    spheres_distance: f32 = math.F32_MAX

    for sphere in spheres {
        current_sphere_intersects, distance_of_i := ray_intersects_sphere(ray, sphere)

        // Finds the closest sphere.
        if (current_sphere_intersects && (distance_of_i < spheres_distance)) {
            spheres_distance = distance_of_i

            hit_point = ray.origin + (ray.direction * distance_of_i)

            surface_nml := hit_point - sphere.center
            surface_normal = linalg.normalize(surface_nml)

            material = sphere.material
        }
    }

    checkerboard_distance: f32 = math.F32_MAX

    if (abs(ray.direction.y) > 1e-3) {
        board_distance := -(ray.origin.y+4) / ray.direction.y // the checkerboard plane has equation y = -4
        board_hit_point := ray.origin + (ray.direction * board_distance)

        if (board_distance > 0 && abs(board_hit_point.x) < 10 && board_hit_point.z < -10 && board_hit_point.z > -30 && board_distance < spheres_distance) {
            checkerboard_distance = board_distance
            hit_point = board_hit_point
            surface_normal = {0, 1, 0}

            white_or_orange := (int(0.5 * hit_point.x + 1000) + int(0.5 * hit_point.z))

            WHITE  :: Vec3f {1.0, 1.0, 1.0}
            ORANGE :: Vec3f {1.0, 0.7, 0.3}
            material.diffuse_color = white_or_orange & 1 > 0 ? WHITE : ORANGE
            material.diffuse_color = material.diffuse_color * 0.3
        }
    }
    intersects = (spheres_distance < 1000) || (checkerboard_distance < 1000)
    return intersects, hit_point, surface_normal, material
}

// Return color of the sphere if intersected, otherwise returns background color.
cast_ray :: proc(
    ray: Ray,
    spheres: []Sphere,
    lights: []Light,
    depth: int
) -> Vec3f {
    if depth > 5 { return BACKGROUND_COLOR }

    intersects, point, surface_normal, material := scene_intersect(ray, spheres)
    if !intersects { return BACKGROUND_COLOR }

    reflect_color: Vec3f
    {
        // Reflection stuff happens in this scope.
        reflect_direction := linalg.normalize(reflection_vector(ray.direction, surface_normal))
    
        // offset the original point to avoid occlusion by the object itself
        offset_direction: f32 = linalg.dot(reflect_direction, surface_normal) < 0 ? -1.0 : 1.0
        reflect_origin: Vec3f = point + (surface_normal * offset_direction * 1e-3)

        reflection_ray: Ray = { reflect_origin, reflect_direction }
        reflect_color = cast_ray(reflection_ray, spheres, lights, depth + 1)
    }

    refract_color: Vec3f
    {
        // refraction stuff happens in this scope.
        refract_direction : Vec3f = refraction_vector(ray.direction, surface_normal, material.refractive_index)
        refract_direction = linalg.normalize(refract_direction)
    
        offset_direction: f32 = linalg.dot(refract_direction, surface_normal) < 0 ? -1.0 : 1.0
        refract_origin: Vec3f = point + (surface_normal * offset_direction * 1e-3)

        refraction_ray: Ray = { refract_origin, refract_direction }
        refract_color = cast_ray(refraction_ray, spheres, lights, depth + 1)
    }

    diffuse_light_intensity, specular_light_intensity: f32
    for light in lights {
        light_direction := light.position - point
        light_direction = linalg.normalize(light_direction)

        {
            // Can this point see the current light?
            light_to_point_vec := light.position - point
            light_distance := linalg.length(light_to_point_vec)

            offset_direction: f32 = linalg.dot(light_direction, surface_normal) < 0 ? -1.0 : 1.0
            shadow_origin: Vec3f = point + (surface_normal * offset_direction * 1e-3)

            light_intersected, shadow_point, shadow_normal, _ := scene_intersect(
                {shadow_origin, light_direction},
                spheres
            )

            pt_to_origin := shadow_point - shadow_origin
            obstruction_closer_than_light := linalg.length(pt_to_origin) < light_distance
            in_shadow := light_intersected && obstruction_closer_than_light
            if in_shadow do continue
        }

        // Diffuse Lighting:
        surface_illumination_intensity := linalg.dot(light_direction, surface_normal)
        if surface_illumination_intensity < 0 { surface_illumination_intensity = 0 }
        diffuse_light_intensity += light.intensity * surface_illumination_intensity

        // Specular Lighting:
        specular_illumination_intensity := linalg.dot(reflection_vector(light_direction, surface_normal), ray.direction)
        if specular_illumination_intensity < 0 { specular_illumination_intensity = 0 }
        specular_illumination_intensity = math.pow(specular_illumination_intensity, material.specular_exponent)
        specular_light_intensity += light.intensity * specular_illumination_intensity
    }

    lighting := (material.diffuse_color * (diffuse_light_intensity * material.albedo.x)) + (Vec3f {1.0, 1.0, 1.0} * (specular_light_intensity * material.albedo.y))

    reflect_refract := (reflect_color * material.albedo.z) + (refract_color * material.albedo.w)

    return lighting + reflect_refract
}

render :: proc(
    spheres: []Sphere,
    lights: []Light,
) {
    framebuffer: []Vec3f = make([]Vec3f, WIDTH * HEIGHT)
    defer delete(framebuffer)

    // Each pixel in the resulting image will have an RGB value, represented by the Vec3f type.
    for row in 0..< HEIGHT {
        for col in 0..< WIDTH {
            // Sweeping the field of view with rays.

            camera_screen_distance : f32 : 1.0
            screen_width := 2.0 * linalg.tan(FOV/2.0) * camera_screen_distance
            x: f32 =  (screen_width * (f32(col) + 0.5)/WIDTH  - 1.0) * f32(WIDTH)/HEIGHT
            y: f32 = -(screen_width * (f32(row) + 0.5)/HEIGHT - 1.0)

            dir: Vec3f = linalg.normalize(Vec3f {x, y, -1})
            ray: Ray = {{0, 0, 0}, dir}

            framebuffer[col + row * WIDTH] = cast_ray(ray, spheres, lights, 0)
        }
    }
    dump_ppm_image(framebuffer, WIDTH, HEIGHT)
}

main :: proc()
{
    ivory:      Material = {1.0, {0.6,  0.3, 0.1, 0.0}, {0.4, 0.4, 0.3},   50.0}
    glass:      Material = {1.5, {0.0,  0.5, 0.1, 0.8}, {0.6, 0.7, 0.8},  125.0}
    red_rubber: Material = {1.0, {0.9,  0.1, 0.0, 0.0}, {0.3, 0.1, 0.1},   10.0}
    mirror:     Material = {1.0, {0.0, 10.0, 0.8, 0.0}, {1.0, 1.0, 1.0}, 1425.0}

    spheres: []Sphere = {
        {{-3,    0,   -16}, 2,      ivory},
        {{-1.0, -1.5, -12}, 2,      glass},
        {{ 1.5, -0.5, -18}, 3, red_rubber},
        {{ 7,    5,   -18}, 4,     mirror},
    }

    lights: []Light = {
        {{-20, 20,  20}, 1.5},
        {{ 30, 50, -25}, 1.8},
        {{ 30, 20,  30}, 1.7},
    }

    render(spheres, lights)
    fmt.println("All done.")
}
