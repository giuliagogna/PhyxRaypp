module;

export module Shape;
import std;
import Geometry;
import Camera;
import auxiliary_functions;
import Color;
import HDRImage;

// ================================================
// HIT RECORD STRUCTURE
// ================================================

export struct HitRecord {
    Ray ray; // Ray that hit the shape
    Point hit_point; // Point of intersection
    Normal hit_normal; // Normal at the intersection point
    std::pair<float, float> uv; // UV coordinates at the intersection point
    float t; // Ray parameter at the intersection point
    bool is_close(const HitRecord& other, float epsilon = 1e-5f) const; // Check if two HitRecords are close enough
};

bool HitRecord::is_close(const HitRecord& other, float epsilon) const {
    return ray.is_close(other.ray, epsilon) &&
           hit_point.is_close(other.hit_point, epsilon) &&
           hit_normal.is_close(other.hit_normal, epsilon) &&
           aux::are_close(uv.first, other.uv.first, epsilon) &&
           aux::are_close(uv.second, other.uv.second, epsilon) &&
           aux::are_close(t, other.t, epsilon);
}

// ======================================================
// SHAPE STRUCTURE (virtual base class for all shapes)
// ======================================================

export struct Shape {
    virtual ~Shape() = default; // Virtual destructor for proper cleanup of derived classes
    virtual std::optional<HitRecord> ray_intersection(const Ray& ray) const = 0; // Pure virtual method to compute ray-shape intersection
};

struct Sphere : Shape {
    Point origin; // Center of the sphere // RP: this would be amazing if it was a Vec...
    float radius; // Radius of the sphere
    Transformation _t;
    Sphere(const Point& origin, float radius) : origin(origin), radius(radius), _t(Scale(Vec(radius, radius, radius)) * Tras(-origin.to_vec())) {} // Constructor

    std::optional<HitRecord> ray_intersection(const Ray& ray) const override; // Override method to compute ray-sphere intersection
};

/// Returns a HitRecord in the axis origin frame if the ray intersects the sphere, std::nullopt otherwise
std::optional<HitRecord> Sphere::ray_intersection(const Ray& ray) const {
    
    Ray ray_sphere = ray.transform(_t); // Transform the ray to the sphere reference frame (where the sphere is a unit sphere centered at the origin)
    // tradeoff: if the most of the rays intersect the sphere, it's better to compute and
    // store direction2 once instead of calling the method everytime.
    float direction2 = ray_sphere.direction.norm2();

    float discriminant = std::pow(ray_sphere.direction * ray_sphere.origin.to_vec(), 2) - direction2 * (ray_sphere.origin.to_vec().norm2() - 1.0f);
    if (discriminant < 0.0f) {
        return std::nullopt; // No intersection
    }

    float sqrt_disc = std::sqrt(discriminant);
    float t1 = (-ray_sphere.direction * ray_sphere.origin.to_vec() - sqrt_disc) / direction2;
    float t2 = (-ray_sphere.direction * ray_sphere.origin.to_vec() + sqrt_disc) / direction2;

    if (t1 > ray_sphere.tmin && t1 < ray_sphere.tmax) {
        Point hit_point = ray.at(t1); // Exiting sphere reference frame
        Normal hit_normal = - ray_sphere.direction.to_norm(); // Exploiting the sphere reference frame
        float u = std::atan2(hit_normal.z, hit_normal.x) / (2.0f * std::numbers::pi_v<float>) + 0.5f;
        float v = std::asin(hit_normal.y) / std::numbers::pi_v<float> + 0.5f;
        return HitRecord{ray, hit_point, hit_normal, {u, v}, t1};

    } else if (t2 > ray_sphere.tmin && t2 < ray_sphere.tmax) {
        Point hit_point = ray.at(t2);
        Normal geom_normal = - ray_sphere.direction.to_norm(); // To be flipped
        float u = std::atan2(geom_normal.z, geom_normal.x) / (2.0f * std::numbers::pi_v<float>) + 0.5f;
        float v = std::asin(geom_normal.y) / std::numbers::pi_v<float> + 0.5f;
        Normal hit_normal = -geom_normal; // Flipped
        return HitRecord{ray, hit_point, hit_normal, {u, v}, t2};
    } else {
        return std::nullopt; // No valid intersection within ray bounds
    }
}