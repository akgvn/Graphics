// Notes:
// TinyRaytracer only uses Vec4f and Vec3f

typedef struct
{
    union {
        float data[2];
        struct
        {
            float x, y;
        };
    };
} Vec2f;

typedef struct
{
    union {
        float data[3];
        struct
        {
            float x, y, z;
        };
    };
} Vec3f;

typedef struct
{
    union {
        float data[4];
        struct
        {
            float x, y, z, w;
        };
    };
} Vec4f;

// Vec4f
float multiply_vec4f(Vec4f* lhs, Vec4f* rhs);
Vec4f multiply_vec4f_with_scalar(Vec4f* lhs, float rhs);
Vec4f add_vec4f(Vec4f* lhs, Vec4f* rhs);
Vec4f sub_vec4f(Vec4f* lhs, Vec4f* rhs);

// Vec3f
float multiply_vec3f(Vec3f* lhs, Vec3f* rhs);
Vec3f multiply_vec3f_with_scalar(Vec3f* lhs, float rhs);
Vec3f add_vec3f(Vec3f* lhs, Vec3f* rhs);
Vec3f sub_vec3f(Vec3f* lhs, Vec3f* rhs);
Vec3f cross(Vec3f* v1, Vec3f* v2);
float vec3f_norm(Vec3f vec);
void vec3f_normalize(Vec3f* vec);

// Vec2f
float multiply_vec2f(Vec2f* lhs, Vec2f* rhs);
Vec2f multiply_vec2f_with_float(Vec2f* lhs, float rhs);
Vec2f add_vec2f(Vec2f* lhs, Vec2f* rhs);
Vec2f sub_vec2f(Vec2f* lhs, Vec2f* rhs);

// Print functions:
void print_vec4f(Vec4f vec);
void print_vec3f(Vec3f vec);
void print_vec2f(Vec2f vec);