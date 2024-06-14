#include "ray_tracing_lib.h"

#include <stdio.h>

#define DEG2RAD 0.017453292

Camera::Camera(vec3 position, vec3 forward, uint32_t width, uint32_t height, float fov, float nearClipPlane) :
    position{ position }, forward{ forward }, fov{ fov }, width{ width }, height{ height }, nearClipPlane{ nearClipPlane }
{
    right = cross(forward, vec3(0, 0, 1));
    up = cross(forward, right);

    projPlaneHeight = nearClipPlane * tan(fov * 0.5f * DEG2RAD) * 2.f;
    projPlaneWidth = projPlaneHeight * ((float) width / height);

    bottomLeftLocal = vec3(nearClipPlane, -projPlaneWidth / 2.f, -projPlaneHeight / 2.f);
}

vec3 camera_ray_direction(int x, int y, const Camera& camera) {
    float tx = x / (static_cast<float>(camera.width)  - 1.f);
    float ty = y / (static_cast<float>(camera.height) - 1.f);

    vec3 pointLocal = camera.bottomLeftLocal + vec3(0, camera.projPlaneWidth * tx, camera.projPlaneHeight * ty);
    vec3 dir = camera.position + camera.right * pointLocal.y + camera.up * pointLocal.z + camera.forward * pointLocal.x;
    dir = glm::normalize(dir);

    return dir;
}

Hittable::Hittable(vec3 position, RayTracingMaterial material) : position{ position }, material{ material } {

}
Sphere::Sphere(vec3 position, float radius, RayTracingMaterial material) : Hittable{ position, material }, radius{ radius } {

}
RayHitInfo Sphere::intersect(Ray ray) {
    RayHitInfo hit;
    vec3 localRayOrigin = ray.origin - position;
    ray.dir = glm::normalize(ray.dir);

    // solving the sphere equation radius = length(origin + direction * distance) for distance in a quadratic equation a * x^2 + b * x + c = 0
    float a = dot(ray.dir, ray.dir);
    float b = 2 * dot(localRayOrigin, ray.dir);
    float c = dot(localRayOrigin, localRayOrigin) - radius * radius;

    float discriminant = b * b - 4 * a * c;

    // if there is a solution for the distance within the real numbers
    if (discriminant >= 0) {
        float distance = (-b - sqrt(discriminant)) / (2 * a);

        if (distance >= 0) {
            hit.didHit = true;
            hit.distance = distance;
            hit.material = material;
            hit.impact = ray.origin + ray.dir * distance;
            hit.normal = normalize(hit.impact - position);
        }
    }

    return hit;
}
RayHitInfo Triangle::intersect(Ray ray) {
    const float EPSILON = 0.0000001;
    vec3 vertex0 = a + position;
    vec3 vertex1 = b + position;  
    vec3 vertex2 = c + position;
    vec3 edge1, edge2, h, s, q;
    float a,f,u,v;
    edge1 = vertex1 - vertex0;
    edge2 = vertex2 - vertex0;
    h = glm::cross(ray.dir, edge2);
    a = glm::dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return { };    // This ray is parallel to this triangle.
    f = 1.0 / a;
    s = ray.origin - vertex0;
    u = f * glm::dot(s, h);
    if (u < 0.0 || u > 1.0)
        return { };
    q = glm::cross(s, edge1);
    v = f * glm::dot(ray.dir, q);
    if (v < 0.0 || u + v > 1.0)
        return { };
    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = f * glm::dot(edge2, q);

    RayHitInfo hit;
    if (t > EPSILON) // ray intersection
    {
        hit.impact = ray.origin + ray.dir * t;
        hit.didHit = true;
        hit.distance = glm::length(ray.dir * t);
        hit.material = material;
        return hit;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return { };
}

RayHitInfo ray_hit(Ray ray, const Scene& scene) {
    RayHitInfo closest;
    for (auto object : scene.objects) {
        RayHitInfo hit = object->intersect(ray);
        if (hit.didHit && hit.distance < closest.distance) {
            closest = hit;
        }
    }
    return closest;
}

vec3 reflect_ray(vec3 incoming, vec3 normal) {
    return incoming - 2 * glm::dot(incoming, normal) * normal;
}

float lerp(float a, float b, float t) {
    return a * (1 - t) + b * t;
}

vec3 lerp(vec3 a, vec3 b, float t) {
    return vec3{ lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t) };
}

std::random_device rand_dev;
std::mt19937 rng{ rand_dev() };
std::normal_distribution<> normal_dist{ 0, 1 };
std::uniform_real_distribution<> uniform_dist{ 0, 1 };

vec3 random_diffusion_direction(vec3 normal) {
    vec3 random = random_vector();
    if (dot(random, normal) < 0)
        return -random;
    else 
        return random;
}

vec3 random_vector() {
    return normalize(vec3{ normal_dist(rng), normal_dist(rng), normal_dist(rng) });
}

float random_value() {
    return uniform_dist(rng);
}

vec3 trace_ray(Ray ray, Scene& scene) {
    vec3 incomingLight { 0 };
    vec3 rayColor { 1 };

    for (int i = 0; i < MAX_RAY_BOUNCE_COUNT; i++) {
        RayHitInfo hit = ray_hit(ray, scene);
        if (hit.didHit) {
            ray.origin = hit.impact;
            auto mat = hit.material;

            vec3 diffuseDir = glm::normalize(hit.normal + random_vector());
            vec3 specularDir = reflect_ray(ray.dir, hit.normal);
            bool isSpecularBounce = mat.glossiness >= random_value();
            ray.dir = lerp(diffuseDir, specularDir, mat.smoothness * isSpecularBounce);

            vec3 emittedLight = mat.emissionColor * mat.emissionStrength;
            incomingLight += vec3(emittedLight.x * rayColor.x, emittedLight.y * rayColor.y, emittedLight.z * rayColor.z);
            rayColor *= mat.color;
        } else {
            break;
        }
    }

    return incomingLight;
}

uint32_t encode_color(vec3 color) {
    return static_cast<uint32_t>(color.r * COLOR_MAX) << 24  |
           static_cast<uint32_t>(color.g * COLOR_MAX) << 16  |
           static_cast<uint32_t>(color.b * COLOR_MAX) << 8 |
           255 << 0;
}