module;

export module Shape;
import std;
import Geometry;
import Camera;
import auxiliary_functions;
import Color;
import HDRImage;

// GG: Potential Vec2D structure
//export struct Vec2d {
//    float u{0.0f}, v{0.0f};
//
//    [[nodiscard]] bool is_close(const Vec2d& other, float epsilon = 1e-5f) const {
//        return aux::are_close(u, other.u, epsilon) &&
//               aux::are_close(v, other.v, epsilon);
//    }
//};

// ================================================
// HIT RECORD STRUCTURE
// ================================================

export struct HitRecord {
    Ray ray; // Ray that hit the shape
    Point hit_point; // Point of intersection
    Normal hit_normal; // Normal at the intersection point
    // GG: Potentially consider to implement a struct Vec2D in order to be able to implement
    // operator overload and call u and v with the name u and v and not uv.first and uv.second
    // Vec2D params // UV coordinates at the intersection point
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

// GG: To change with the one RP is writing
// ================================================================
// SHAPE BASE CLASS (GG: just used to implement the next classes)
// ================================================================
export struct Shape {
    Transformation trans;
    Shape(const Transformation& trans = Transformation{}) : trans(trans) {}
    virtual ~Shape() = default;
    [[nodiscard]] virtual std::optional<HitRecord> ray_intersection(const Ray& ray) const = 0;
};

// ==================================
// PLANE
// ==================================
export struct Plane : Shape {
    // Inherits constructors from Shape (allows passing Transformation)
    using Shape::Shape;

    [[nodiscard]] std::optional<HitRecord> ray_intersection(const Ray& ray) const override {

        // Reference frame change: form World to local Shape frame
        Ray local_ray = ray.transform(trans.inverse());

        // Control parallelism: if z-component of ray direction is close to zero there is no intersection
        if (std::abs(local_ray.direction.z) < 1e-5f) {
            return std::nullopt;
        }

        // Intersection: O_z + t * d_z = 0  =>  t = -O_z / d_z
        float t = -local_ray.origin.z / local_ray.direction.z;

        // t limits control
        if (t < local_ray.tmin || t > local_ray.tmax) {
            return std::nullopt;
        }

        // Point of intersection in the frame of reference of the plane
        Point local_point = local_ray.at(t);

        // Return in the World space (apply direct transformation)
        HitRecord record;
        record.ray = ray;
        record.t = t;
        record.hit_point = trans * local_point;

        Normal local_normal = (local_ray.direction.z < 0) ? Normal{0.0f, 0.0f, 1.0f} : Normal{0.0f, 0.0f, -1.0f};
        record.hit_normal = trans * local_normal;

        // Record (u, v) coordinates on the pair (Tile Pattern: represent infinite plane as composition of finite tiles)
        // - floor(-3.2) = -4
        record.uv = {
            local_point.x - std::floor(local_point.x),
            local_point.y - std::floor(local_point.y)
        };

        return record;
    }
};
