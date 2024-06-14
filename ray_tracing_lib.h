#pragma once

#include <vector>
#include <random>

#include <glm/glm.hpp>
using vec3 = glm::vec3;
using vec4 = glm::vec4;

#define MAX_RAY_BOUNCE_COUNT 10
#define RAYS_PER_PIXEL 20

// material for certain ray reflection properties
typedef struct {
    vec3 color;
    vec3 emissionColor;
    float emissionStrength;
    float smoothness;
    float glossiness;
} RayTracingMaterial;

// Information struct for saving Ray info
typedef float ray_distance_t;
typedef struct RayHitInfo {
    ray_distance_t distance;
    vec3 impact;
    vec3 normal;
    RayTracingMaterial material;
    bool didHit;

    RayHitInfo() : distance{ 10000.f }, impact{ }, normal{ }, material{ }, didHit{ false } {

    }
} RayHitInfo;

// ray struct
typedef struct {
    vec3 origin;
    vec3 dir;
} Ray;

// virtual class that can be derived for a class to be hit by a ray
class Hittable {
public:
    vec3 position;
    RayTracingMaterial material;
    virtual RayHitInfo intersect(Ray ray) = 0;
    Hittable(vec3 position, RayTracingMaterial material);
};

// guess what... a TRIANGLE struct (OMG What a surprise!!!)
class Triangle : public Hittable {
public:
    vec3 a, b, c;
    Triangle(vec3 pos, vec3 a, vec3 b, vec3 c, RayTracingMaterial material) : Hittable { pos, material }, a { a }, b { b }, c { c } {}
    RayHitInfo intersect(Ray ray) override;
};

// hittable triangle mesh for ray-intersection
class Mesh : public Hittable {
public:
    std::vector<Triangle> triangles;
    RayHitInfo intersect(Ray ray) override;
};

// hittable sphere for ray-intersection 
class Sphere : public Hittable {
public:
    float radius;
    RayHitInfo intersect(Ray ray) override;
    Sphere(vec3 position, float radius, RayTracingMaterial material);
};

// scene struct for saving ray-hittable objects
typedef struct {
    std::vector<Hittable*> objects;
} Scene;

// camera struct
typedef struct Camera {
    vec3 position;
    vec3 forward;
    vec3 up;
    vec3 right;
    float fov;
    uint32_t width;
    uint32_t height;
    float nearClipPlane;
    float projPlaneHeight;
    float projPlaneWidth;
    vec3 bottomLeftLocal;

    Camera(vec3 position, vec3 facing, uint32_t width, uint32_t height, float fov = 90.f, float nearClipPlane = 0.1f);
} Camera;

// function to find out the direction of a ray corresponding to a certain pixel
vec3 camera_ray_direction(int x, int y, const Camera& camera);

// function to shoot out a ray into a scene
RayHitInfo ray_hit(Ray ray, const Scene& scene);

// function to trace rays with light
vec3 trace_ray(Ray ray, Scene& scene);

// function to encode colors (vec3) to bit values
#define COLOR_MAX 255
uint32_t encode_color(vec3 color);

vec3 random_vector();
vec3 random_diffusion_direction(vec3 normal);