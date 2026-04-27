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
    // GG: Potentially consider to implement a struct Vec2D in order to be able to implement
    // operator overload and call u and v with the name u and v and not uv.first and uv.second

    // RP: "params" don't let you understand what they actually are IMO. "surface_params" is a blend of
    // the name in the lectures and "params" which I agree is more coherent with our code.
    //Vec2D params; // UV coordinates at the intersection point
    Vec2D surface_params; // UV coordinates at the intersection point
    float t; // Ray parameter at the intersection point
    bool is_close(const HitRecord& other, float epsilon = 1e-5f) const; // Check if two HitRecords are close enough
};

bool HitRecord::is_close(const HitRecord& other, float epsilon) const {
    return ray.is_close(other.ray, epsilon) &&
           hit_point.is_close(other.hit_point, epsilon) &&
           hit_normal.is_close(other.hit_normal, epsilon) &&
           aux::are_close(surface_params.u, other.surface_params.u, epsilon) &&
           aux::are_close(surface_params.v, other.surface_params.v, epsilon) &&
           //aux::are_close(uv.first, other.uv.first, epsilon) &&
           //aux::are_close(uv.second, other.uv.second, epsilon) &&
           aux::are_close(t, other.t, epsilon);
}

// ======================================================
// SHAPE STRUCTURE (virtual base class for all shapes)
// ======================================================

export struct Shape {
    // GG: Constructor in Shape needs to be initialized with a transformation.
    //     The object is put into the scene in the right location: responsibility of calculations is delegated
    //     to the object that performs them in its frame of reference
    Shape(const Transformation& trans = Transformation{}) : trans(trans) {}
    virtual ~Shape() = default; // Virtual destructor for proper cleanup of derived classes
    virtual std::optional<HitRecord> ray_intersection(const Ray& ray) const = 0; // Pure virtual method to compute ray-shape intersection
    Transformation trans; // Transformation from the shape's local space to world space (position and orientation of the shape in the scene)
};


// ======================================================
// SPHERE
// ======================================================
export struct Sphere : Shape {
    Sphere(const Point& origin, float radius) : Shape(Scale(Vec{1.0f/radius, 1.0f/radius, 1.0f/radius}) * Trans(-origin.to_vec())) {} // Constructor

    std::optional<HitRecord> ray_intersection(const Ray& ray) const override; // Override method to compute ray-sphere intersection
};

/// Returns a HitRecord in the axis origin frame if the ray intersects the sphere, std::nullopt otherwise
std::optional<HitRecord> Sphere::ray_intersection(const Ray& ray) const {

    Ray ray_sphere = ray.transform(trans); // Transform the ray to the sphere reference frame (where the sphere is a unit sphere centered at the origin)
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
        Normal hit_normal = ray_sphere.at(t1).to_norm(); // Exploiting the sphere reference frame
        float u = std::atan2(hit_normal.y, hit_normal.x) / (2.0f * std::numbers::pi_v<float>) + 0.5f; // atan2 returns values in the range [-pi, pi], we want to map it to [0, 1]
        float v = std::acos(hit_normal.z) / std::numbers::pi_v<float>;
        return HitRecord{ray, hit_point, hit_normal, {u, v}, t1};

    } else if (t2 > ray_sphere.tmin && t2 < ray_sphere.tmax) {
        Point hit_point = ray.at(t2);
        Normal geom_normal = ray_sphere.at(t2).to_norm(); // To be flipped
        float u = std::atan2(geom_normal.y, geom_normal.x) / (2.0f * std::numbers::pi_v<float>) + 0.5f; // atan2 returns values in the range [-pi, pi], we want to map it to [0, 1]
        float v = std::acos(geom_normal.z) / std::numbers::pi_v<float>;
        Normal hit_normal = -geom_normal; // Flipped
        return HitRecord{ray, hit_point, hit_normal, {u, v}, t2};
    } else {
        return std::nullopt; // No valid intersection within ray bounds
    }
}


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
        record.surface_params.u = local_point.x - std::floor(local_point.x);
        record.surface_params.v = local_point.y - std::floor(local_point.y);
        //record.uv = {
        //    local_point.x - std::floor(local_point.x),
        //    local_point.y - std::floor(local_point.y)
        //};

        return record;
    }
};


// ===================================================================================
// WORLD STRUCT
// ===================================================================================

export struct World {
    std::vector<std::unique_ptr<Shape>> shapes;
    void add(std::unique_ptr<Shape> shape);
    [[nodiscard]] std::optional<HitRecord> ray_intersection(const Ray& ray) const;
};

// Add a shape to the World: use unique_ptr + move pattern for optimization
void World::add(std::unique_ptr<Shape> shape) {
    shapes.push_back(std::move(shape));
}

[[nodiscard]] std::optional<HitRecord> World::ray_intersection(const Ray& ray) const {
    // Default: no hit
    std::optional<HitRecord> closest = std::nullopt;

    Ray ray_copy = ray; // At this point we just do like this, but it would be possible that
                        // we will just pass ray as non-const reference
                        // We could also directly pass ray by copy.
    // Cicle on shapes in the scene
    for (const auto& shape : shapes) {

        if (auto intersection = shape->ray_intersection(ray_copy)){
            closest = intersection;
            ray_copy.tmax = closest->t; // Update tmax to the closest hit
            // RP: I'll just change the tmax! In this way I avoid to actually generate
            // the HitRecord object and then throw it away.
            // If we hit we also update both closest and tmax.
            // If we never hit a shape, there's the default std::nullopt return.
        }        
    }
    return closest;
}

