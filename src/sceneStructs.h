#pragma once

#include <string>
#include <vector>
#include <cuda_runtime.h>
#include "glm/glm.hpp"

#include "common.h"

#define BACKGROUND_COLOR (glm::vec3(0.0f))

enum GeomType {
    SPHERE,
    CUBE,
};

struct Ray 
{
    glm::vec3 origin;
    glm::vec3 direction;
    CPU_GPU Ray(const glm::vec3& o = {0, 0, 0}, const glm::vec3& d = { 0, 0, 0 })
        :origin(o), direction(d)
    {}

    CPU_GPU glm::vec3 operator*(const float& t) const { return origin + t * direction; }

public:
    CPU_GPU static Ray SpawnRay(const glm::vec3& o, const glm::vec3& dir)
    {
        return { o + dir * 0.001f, dir };
    }
};

struct Geom {
    enum GeomType type;
    int materialid;
    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::mat4 transform;
    glm::mat4 inverseTransform;
    glm::mat4 invTranspose;
};

struct Material {
    glm::vec3 albedo{0.f};
    float emittance = 0.f;
};

struct Camera {
    glm::ivec2 resolution;
    glm::vec3 position;
    glm::vec3 ref;
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;
    float fovy;
    glm::vec2 pixelLength;

    CPU_ONLY void Recompute() 
    {
        forward = glm::normalize(ref - position);
        right = glm::normalize(glm::cross(forward, {0, 1, 0}));
        up = glm::normalize(glm::cross(right, forward));
    }
    CPU_GPU Ray CastRay(const glm::vec2& p)
    {
        glm::vec2 ndc = 2.f * p / glm::vec2(resolution); 
        ndc.x = ndc.x - 1.f;
        ndc.y = 1.f - ndc.y;

        float aspect = static_cast<float>(resolution.x) / static_cast<float>(resolution.y);

        // point in camera space
        float radian = glm::radians(fovy * 0.5f);
        glm::vec3 p_camera = glm::vec3(
            ndc.x * glm::tan(radian) * aspect,
            ndc.y * glm::tan(radian),
            1.f
        );

        Ray ray(glm::vec3(0), p_camera);

        // transform to world space
        ray.origin = position + ray.origin.x * right + ray.origin.y * up;
        ray.direction = glm::normalize(
            ray.direction.z * forward +
            ray.direction.y * up +
            ray.direction.x * right
        );

        return ray;
    }
};

struct RenderState {
    Camera camera;
    unsigned int iterations;
    int traceDepth;
    std::vector<glm::vec3> image;
    std::vector<uchar4> c_image;
    std::string imageName;
};

struct PathSegment {
    Ray ray;
    glm::vec3 throughput{ 1, 1, 1 };
    glm::vec3 radiance{0, 0, 0};
    int pixelIndex;
    int remainingBounces;
    CPU_GPU void Reset() 
    {
        throughput = glm::vec3(1.f);
        radiance = glm::vec3(0.f);
        pixelIndex = 0;
    }
    CPU_GPU void Terminate() { remainingBounces = 0; }
    CPU_GPU bool IsEnd() const { return remainingBounces <= 0; }
};

struct Intersection
{
    int shapeId;
    int materialId;
    float t;
    glm::vec2 uv; // local uv
};

// Use with a corresponding PathSegment to do:
// 1) color contribution computation
// 2) BSDF evaluation: generate a new ray
struct ShadeableIntersection 
{
    float t;
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    int materialId;

    CPU_GPU void Reset()
    {
        t = -1.f;
        materialId = -1.f;
    }
};
