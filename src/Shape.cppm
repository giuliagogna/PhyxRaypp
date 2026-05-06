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
           aux::are_close(t, other.t, epsilon);
}

// ======================================================
// SHAPE STRUCTURE (virtual base class for all shapes)
// ======================================================

export struct AABB {
    float x_min = std::numeric_limits<float>::infinity(), x_max = - std::numeric_limits<float>::infinity();
    float y_min = std::numeric_limits<float>::infinity(), y_max = - std::numeric_limits<float>::infinity();
    float z_min = std::numeric_limits<float>::infinity(), z_max = - std::numeric_limits<float>::infinity();

    bool is_inside(AABB& greater) const {
        if (x_max < greater.x_max &&
            x_min > greater.x_min &&
            y_max < greater.y_max &&
            y_min > greater.y_min &&
            z_max < greater.z_max &&
            z_min > greater.z_min
            ) return true;
        return false;
    }

    void expand_to_include(const AABB& other) {
        this->x_max = std::max(other.x_max, this->x_max);
        this->x_min = std::min(other.x_min, this->x_min);
        this->y_max = std::max(other.y_max, this->y_max);
        this->y_min = std::min(other.y_min, this->y_min);
        this->z_max = std::max(other.z_max, this->z_max);
        this->z_min = std::min(other.z_min, this->z_min);
    }

};

export struct Shape {
    // GG: Constructor in Shape needs to be initialized with a transformation.
    //     The object is put into the scene in the right location: responsibility of calculations is delegated
    //     to the object that performs them in its frame of reference
    Shape(const Transformation& trans = Transformation{}) : trans(trans) {}
    virtual ~Shape() = default; // Virtual destructor for proper cleanup of derived classes
    virtual std::optional<HitRecord> ray_intersection(const Ray& ray) const = 0; // Pure virtual method to compute ray-shape intersection
    virtual AABB get_AABB(float padding = 1e-3) const = 0; // Pure virtual method to get the AABB of the shape (used for AABB construction)
    Transformation trans; // Transformation from the shape's local space to world space (position and orientation of the shape in the scene)
    bool is_inside(AABB& aabb, float padding = 1e-3) {
        return this->get_AABB().is_inside(aabb);
    }
};

// ======================================================
// SPHERE
// ======================================================
export struct Sphere : Shape {
    using Shape::Shape; // Constructor takes in the Shape transformation

    /// Returns a HitRecord in the axis origin frame if the ray intersects the sphere, std::nullopt otherwise
    std::optional<HitRecord> ray_intersection(const Ray& ray) const override { // Override method to compute ray-sphere intersection

        // World Space -> Local Space
        Ray local_ray = ray.transform(trans.inverse()); // Transform the ray to the sphere reference frame (where the sphere is a unit sphere centered at the origin)
                                                        // GG: to do so you need to use the inverse transformation!

        // local_ray
        Vec O = local_ray.origin.to_vec(); // ray origin is stored as a Vec, so it can be normalized
                                           // (under the carpet the calculation is Point ray_origin - Point axes_origin = Vec ray_origin)
        Vec D = local_ray.direction;
        // tradeoff: if the most of the rays intersect the sphere, it's better to compute and
        // store direction2 once instead of calling the method everytime.
        float dir2 = D.norm2();

        float b_half = O * D;
        float discriminant = (b_half * b_half) - dir2 * (O.norm2() - 1.0f);

        if (discriminant < 0.0f) {
            return std::nullopt; // No intersection
        }

        float sqrt_disc = std::sqrt(discriminant);
        float t1 = (-b_half - sqrt_disc) / dir2;
        float t2 = (-b_half + sqrt_disc) / dir2;

        // Find the closest valid t
        float first_hit_t;

        if (t1 > local_ray.tmin && t1 < local_ray.tmax) {
            first_hit_t = t1;
        } else if (t2 > local_ray.tmin && t2 < local_ray.tmax) {
            first_hit_t = t2;
        } else {
            return std::nullopt; // Both t1 and t2 are out of bounds
        }

        // Local Space Geometry
        Point local_point = local_ray.at(first_hit_t);

        // Since the calculations assume to have a unit sphere centered at the origin
        // the coordinates of the point of intersection are exactly the components of
        // the normal to the sphere in that point
        Normal local_normal{local_point.x, local_point.y, local_point.z};
        if (local_point.to_vec() * D > 0.0f) {
            local_normal = -local_normal;
        }

        // UV Coordinates MUST be calculated using the local normal.
        // If you use the global normal, the texture won't rotate when you rotate the sphere!
        float u = std::atan2(local_point.y, local_point.x) / (2.0f * std::numbers::pi_v<float>);
        u = (u >= 0.0f) ? u : (u + 1.0f);
        float v = std::acos(local_point.z) / std::numbers::pi_v<float>;

        // Local Space -> World Space: report the HitRecord in the global space
        HitRecord record;
        record.ray = ray;
        record.t = first_hit_t;
        // Report point of intersection and normal in the global space
        record.hit_point = trans * local_point;
        // Normals transform with the transposed inverse: when operator * acts between a Transformation
        // and a Normal it acts using the inverse transposed (see "Transformation of a Normal" in Geometry.cppm
        // for reference).
        // N.B.: if a scale transformation is applied the normal is no more normalized.
        // Normalize it before return
        record.hit_normal = (trans * local_normal).normalize();
        record.surface_params = {u, v};

        return record;
    }

    AABB get_AABB(float padding = 1e-3) const override {
        // For the sphere we have to consider that the transformatiom
        // might carry a rotation which doesn't affect the sphere but affects the AABB.
        // We have to focus on matrix elements
        float x_dil = std::sqrt(trans.m.mat[0]*trans.m.mat[0] + trans.m.mat[1]*trans.m.mat[1] + trans.m.mat[2]*trans.m.mat[2]);
        float y_dil = std::sqrt(trans.m.mat[4]*trans.m.mat[4] + trans.m.mat[5]*trans.m.mat[5] + trans.m.mat[6]*trans.m.mat[6]);
        float z_dil = std::sqrt(trans.m.mat[8]*trans.m.mat[8] + trans.m.mat[9]*trans.m.mat[9] + trans.m.mat[10]*trans.m.mat[10]);

        // Traslazione (posizione del centro nel mondo)
        float tx = trans.m.mat[3];
        float ty = trans.m.mat[7];
        float tz = trans.m.mat[11];

        return AABB { 
            .x_min = tx - x_dil - padding, .x_max = tx + x_dil + padding,
            .y_min = ty - y_dil - padding, .y_max = ty + y_dil + padding,
            .z_min = tz - z_dil - padding, .z_max = tz + z_dil + padding       
        };
    }
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
        record.surface_params.u = local_point.x - std::floor(local_point.x);
        record.surface_params.v = local_point.y - std::floor(local_point.y);

        return record;
    }

    // Should not be used since it's a infinite element
    AABB get_AABB(float padding = 1e-3) const override {
        return AABB{};
    }
};


// ===================================================================================
// WORLD STRUCT
// ===================================================================================

export struct World {
    std::vector<std::unique_ptr<Shape>> shapes;
    // Add a shape to the World: use unique_ptr + move pattern for optimization
    void add(std::unique_ptr<Shape> shape) {
        shapes.push_back(std::move(shape));
    }

    [[nodiscard]] std::optional<HitRecord> ray_intersection(Ray ray) const {
        // Default: no hit
        std::optional<HitRecord> closest = std::nullopt;

        // GG: I find passing by copy is the best way to go. Passing it by reference, the original ray gets
        //     permanently modified making it harder to debug if tmax has changed

        // Cycle on shapes in the scene
        for (const auto& shape : shapes) {
            if (auto intersection = shape->ray_intersection(ray)){
                closest = intersection;
                ray.tmax = closest->t; // Update tmax to the closest hit
                // RP: I'll just change the tmax! In this way I avoid to actually generate
                // the HitRecord object and then throw it away.
                // If we hit we also update both closest and tmax.
                // If we never hit a shape, there's the default std::nullopt return.
                // GG: This is great!
            }
        }
        return closest;
    }
};

