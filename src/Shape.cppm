/*
* Copyright (c) 2026 Giulia Gogna, Riccardo Piazza.
 *
 * Licensed under the EUPL, Version 1.2 or – as soon they will be approved by
 * the European Commission - subsequent versions of the EUPL (the "Licence");
 * You may not use this work except in compliance with the Licence.
 * You may obtain a copy of the Licence at:
 *
 * https://joinup.ec.europa.eu/collection/eupl/eupl-text-eupl-12
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the Licence is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the Licence for the specific language governing permissions and
 * limitations under the Licence.
 */

/**
 * @file Shape.cppm
 * @brief Geometric primitives and ray-scene intersection routines.
 *
 * This module defines the shapes that can be placed in a scene, the information returned by ray intersections,
 * and the World container used to store scene geometry.
 */

module;

export module Shape;
import std;
import Geometry;
import Camera;
import auxiliary_functions;
import Color;
import HDRImage;
import Material;
import Pigment;
import BRDF;

// ================================================
// HIT RECORD STRUCTURE
// ================================================

// Forward declaration
export struct Shape;

/**
 * @brief Information associated with a ray-shape intersection.
 *
 * A HitRecord stores the geometric data required for shading and ray propagation after an intersection has been found.
 */
export struct HitRecord {
    /// Ray that produced the intersection.
    Ray ray;

    /// Intersection point in world space.
    Point hit_point;

    /// Surface normal at the intersection point.
    Normal hit_normal;

    /// Surface parametric coordinates (u,v).
    Vec2D surface_params;

    /// Ray parameter corresponding to the intersection.
    float t;

    /// Shape that was intersected.
    const Shape* hitted_shape = nullptr;

    /// Checks if two HitRecord objects are close within a custom epsilon (used for tests)
    bool is_close(const HitRecord& other, float epsilon = 1e-5f) const; // Check if two HitRecords are close enough
};

bool HitRecord::is_close(const HitRecord& other, float epsilon) const {
    return ray.is_close(other.ray, epsilon) &&
           hit_point.is_close(other.hit_point, epsilon) &&
           hit_normal.is_close(other.hit_normal, epsilon) &&
           aux::are_close(surface_params.u, other.surface_params.u, epsilon) &&
           aux::are_close(surface_params.v, other.surface_params.v, epsilon) &&
           aux::are_close(t, other.t, epsilon) &&
           hitted_shape == other.hitted_shape;
}

/**
 * @brief It adds to HitRecord the information about the direction of the ray, telling if it's entering or exiting the shape.
 */
export struct directionalHitRecord : HitRecord {
    /// Hit information indicating if the ray is entering the surface of hitted_shape.
    bool is_entering;
};

// ======================================================
// SHAPE STRUCTURE (virtual base class for all shapes)
// ======================================================

/**
 * @brief Base class for all geometric primitives.
 *
 * A shape provides a ray-intersection routine and stores the transformation and material associated with the object.
 */
export struct Shape {
    /// Local-to-world transformation.
    Transformation trans;

    /// Material assigned to the shape.
    std::shared_ptr<Material> material;

    Shape(const Transformation& trans = Transformation{}, std::shared_ptr<Material> material = nullptr) : trans(trans), material(material) {}

    /// Virtual destructor.
    virtual ~Shape() = default;

    /**
     * @brief Compute the closest intersection between a ray and the shape.
     *
     * @param ray Ray to test.
     * @return HitRecord object if a hit occurs, nullptr otherwise.
     */
    virtual std::optional<HitRecord> ray_intersection(const Ray& ray) const = 0; // Pure virtual method to compute ray-shape intersection

    /**
     * @brief Compute all the valid intersections between a ray and the shape and store in a vector. Ideal to perform CSG.
     *
     * @param ray Ray to test.
     * @return A vector of directionalHitRecord objects 
     */
    virtual std::vector<directionalHitRecord> ray_all_intersections(const Ray& ray) const = 0; // Pure virtual method to compute ray-shape intersections    
};

// ======================================================
// SPHERE
// ======================================================

/**
 * @brief Unit sphere centered at the local origin.
 *
 * Intersections are computed in object space and then transformed back to world space.
 */
export struct Sphere : Shape {
    using Shape::Shape; // Constructor takes in the Shape transformation

    /// Supports both ray_intersection and ray_all_intersections in finding t1 and t2 equation roots.
    std::array<float, 2> _compute_t1_t2(const Ray& local_ray) const {
        Vec O = local_ray.origin.to_vec(); // ray origin is stored as a Vec, so it can be normalized
                                           // (under the carpet the calculation is Point ray_origin - Point axes_origin = Vec ray_origin)
        Vec D = local_ray.direction;

        float dir2 = D.norm2();

        float b_half = O * D;
        float discriminant = (b_half * b_half) - dir2 * (O.norm2() - 1.0f);

        if (discriminant < 0.0f) {
            return std::array<float, 2>{-1.0f, -1.0f}; // Defaulting to negative, meaningless, values
        }

        float sqrt_disc = std::sqrt(discriminant);
        float t1 = (-b_half - sqrt_disc) / dir2;
        float t2 = (-b_half + sqrt_disc) / dir2;
        return std::array<float, 2>{t1, t2};
    }

    /// Returns a HitRecord in the axis origin frame if the ray intersects the sphere, std::nullopt otherwise
    [[nodiscard]] std::optional<HitRecord> ray_intersection(const Ray& ray) const { // Override method to compute ray-sphere intersection

        // Transform the ray to the sphere reference frame (where the sphere is a unit sphere centered at the origin)
        Ray local_ray = ray.transform(trans.inverse());

        // Find the closest valid t
        float first_hit_t;
        std::array<float, 2> t = _compute_t1_t2(local_ray); // Computes equation roots

        // First check the t[0] since it's always the lower
        if (t[0] > local_ray.tmin && t[0] < local_ray.tmax) {
            first_hit_t = t[0];
        } else if (t[1] > local_ray.tmin && t[1] < local_ray.tmax) {
            first_hit_t = t[1];
        } else {
            return std::nullopt; // Both t1 and t2 are out of bounds
        }

        // Local Space Geometry
        Point local_point = local_ray.at(first_hit_t);

        // Since the calculations assume to have a unit sphere centered at the origin the coordinates of the point of
        // intersection are exactly the components of the normal to the sphere in that point
        Normal local_normal{local_point.x, local_point.y, local_point.z};
        if (local_point.to_vec() * local_ray.direction > 0.0f) {
            local_normal = -local_normal;
        }

        // Compute texture coordinates in local space so that textures follow object transformations.
        float u = std::atan2(local_point.y, local_point.x) / (2.0f * std::numbers::pi_v<float>);
        u = (u >= 0.0f) ? u : (u + 1.0f);
        float v = std::acos(local_point.z) / std::numbers::pi_v<float>;

        // Local Space -> World Space: report the HitRecord in the global space
        HitRecord record;
        record.ray = ray;
        record.t = first_hit_t;
        record.hit_point = trans * local_point;

        // Normals are transformed using the inverse transpose transformation.
        // Renormalize after transformation to account for scaling.
        record.hit_normal = (trans * local_normal).normalize();
        record.surface_params = {u, v};
        record.hitted_shape = this;

        return record;
    }

    /// Returns a vector of directionalHitRecord in the axis origin frame if the ray intersects the sphere, a empty vector otherwise.
    [[nodiscard]] std::vector<directionalHitRecord> ray_all_intersections(const Ray& ray) const {
        // Transform the ray to the sphere reference frame (where the sphere is a unit sphere centered at the origin)
        Ray local_ray = ray.transform(trans.inverse());
        // vector to be filled of intersections
        std::vector<directionalHitRecord> intersections;

        // Find valid t values
        std::array<float, 2> computed_t1_t2 = _compute_t1_t2(local_ray); // Computes equation roots

        // Check if those are suitable
        for (float t : computed_t1_t2) {
            if (t > local_ray.tmin && t < local_ray.tmax) {
                // Local Space Geometry
                Point local_point = local_ray.at(t);

                // Since the calculations assume to have a unit sphere centered at the origin the coordinates of the point of
                // intersection are exactly the components of the normal to the sphere in that point
                Normal local_normal{local_point.x, local_point.y, local_point.z};
                bool is_entering = true;
                if (local_point.to_vec() * local_ray.direction > 0.0f) {
                    local_normal = -local_normal;
                    is_entering = false;
                }

                // Compute texture coordinates in local space so that textures follow object transformations.
                float u = std::atan2(local_point.y, local_point.x) / (2.0f * std::numbers::pi_v<float>);
                u = (u >= 0.0f) ? u : (u + 1.0f);
                float v = std::acos(local_point.z) / std::numbers::pi_v<float>;

                // Local Space -> World Space: report the HitRecord in the global space
                directionalHitRecord record;
                record.ray = ray;
                record.t = t;
                record.hit_point = trans * local_point;

                // Normals are transformed using the inverse transpose transformation.
                // Renormalize after transformation to account for scaling.
                record.hit_normal = (trans * local_normal).normalize();
                record.surface_params = {u, v};
                record.hitted_shape = this;
                record.is_entering = is_entering;

                // Update the vector
                intersections.push_back(record);
            }
        }
        return intersections;
    }

};

// ==================================
// PLANE
// ==================================

/**
 * @brief Infinite plane lying on the local z = 0 plane.
 */
export struct Plane : Shape {

    using Shape::Shape; // Inherits constructors from Shape (allows passing Transformation)

    /// Returns a HitRecord in the axis origin frame if the ray intersects the plane, std::nullopt otherwise
    [[nodiscard]] std::optional<HitRecord> ray_intersection(const Ray& ray) const override {

        // Reference frame change: form World to local Shape frame
        Ray local_ray = ray.transform(trans.inverse());

        // Control parallelism: if z-component of ray direction is close to zero there is no intersection
        if (std::abs(local_ray.direction.z) < 1e-5f) {
            return std::nullopt;
        }

        // Intersection: O_z + t * d_z = 0  =>  t = -O_z / d_z
        float first_hit_t = -local_ray.origin.z / local_ray.direction.z;

        // t limits control
        if (first_hit_t < local_ray.tmin || first_hit_t > local_ray.tmax) {
            return std::nullopt;
        }

        // Return geometric information for intersection in global space
        Point local_point = local_ray.at(first_hit_t);
        directionalHitRecord record;
        record.ray = ray;
        record.t = first_hit_t;
        record.hit_point = trans * local_point;
        Normal local_normal = (local_ray.direction.z < 0) ? Normal{0.0f, 0.0f, 1.0f} : Normal{0.0f, 0.0f, -1.0f};
        record.hit_normal = trans * local_normal;

        // Generate repeating texture coordinates from local position.
        record.surface_params.u = local_point.x - std::floor(local_point.x);
        record.surface_params.v = local_point.y - std::floor(local_point.y);
        record.hitted_shape = this;

        return record;
    }

    /// Returns a vector of directionalHitRecord in the axis origin frame if the ray intersects the plane, a empty vector otherwise. Internal part for this shape is choosen to be z<0 in the plane reference frame.
    [[nodiscard]] std::vector<directionalHitRecord> ray_all_intersections(const Ray& ray) const override {

        // Reference frame change: form World to local Shape frame
        Ray local_ray = ray.transform(trans.inverse());

        // vector to be returned
        std::vector<directionalHitRecord> intersection;

        // Control parallelism: if z-component of ray direction is close to zero there is no intersection
        if (std::abs(local_ray.direction.z) < 1e-5f) {
            return intersection;
        }

        // Intersection: O_z + t * d_z = 0  =>  t = -O_z / d_z
        float t = -local_ray.origin.z / local_ray.direction.z;

        // t limits control
        if (t < local_ray.tmin || t > local_ray.tmax) {
            return intersection;
        }

        // Return geometric information for intersection in global space
        Point local_point = local_ray.at(t);
        directionalHitRecord record;
        record.ray = ray;
        record.t = t;
        record.hit_point = trans * local_point;
        Normal local_normal = (local_ray.direction.z < 0) ? Normal{0.0f, 0.0f, 1.0f} : Normal{0.0f, 0.0f, -1.0f};
        record.hit_normal = trans * local_normal;

        // Generate repeating texture coordinates from local position
        record.surface_params.u = local_point.x - std::floor(local_point.x);
        record.surface_params.v = local_point.y - std::floor(local_point.y);
        record.hitted_shape = this;
        record.is_entering = (local_normal.z > 0.0f); // If the ray comes from above the plane, then it's entering
        
        intersection.push_back(record);

        return intersection;
    }
};

// ==================================
// CUBE
// =================================

/** @brief Axis-aligned cube spanning [-1,1]^3 in local space.
*
* Intersections are computed in object space using the slab method.
* Surface coordinates are generated by projecting the hit point onto
* the intersected face and mapping the six faces onto a cross-shaped atlas.
*/
export struct Cube : Shape {
    using Shape::Shape;

    [[nodiscard]] std::optional<HitRecord> ray_intersection(const Ray& ray) const override {

        // Transform the ray into the cube's local reference frame
        Ray local_ray = ray.transform(trans.inverse());

        // Ray-box intersection using the slab method
        Vec inv_dir = Vec{
            1.0f / local_ray.direction.x,
            1.0f / local_ray.direction.y,
            1.0f / local_ray.direction.z
        };

        // ---------------------------------------------------------
        // Slab Method (Branchless)
        // ---------------------------------------------------------
        float tx1 = (-1.0f - local_ray.origin.x) * inv_dir.x;
        float tx2 = ( 1.0f - local_ray.origin.x) * inv_dir.x;
        float tmin = std::min(tx1, tx2);
        float tmax = std::max(tx1, tx2);

        float ty1 = (-1.0f - local_ray.origin.y) * inv_dir.y;
        float ty2 = ( 1.0f - local_ray.origin.y) * inv_dir.y;
        tmin = std::max(tmin, std::min(ty1, ty2));
        tmax = std::min(tmax, std::max(ty1, ty2));

        float tz1 = (-1.0f - local_ray.origin.z) * inv_dir.z;
        float tz2 = ( 1.0f - local_ray.origin.z) * inv_dir.z;
        tmin = std::max(tmin, std::min(tz1, tz2));
        tmax = std::min(tmax, std::max(tz1, tz2));

        if (tmin > tmax) return std::nullopt;

        // Choose the first valid intersection along the ray
        float first_hit_t{0.0f};
        if (tmin > local_ray.tmin && tmin < local_ray.tmax) {
            first_hit_t = tmin;
        } else if (tmax > local_ray.tmin && tmax < local_ray.tmax) {
            first_hit_t = tmax;
        } else {
            return std::nullopt;
        }

        Point local_point = local_ray.at(first_hit_t);
        Normal local_normal{0.0f, 0.0f, 0.0f};

        // The dominant coordinate identifies the intersected face
        float abs_x = std::abs(local_point.x);
        float abs_y = std::abs(local_point.y);
        float abs_z = std::abs(local_point.z);

        /* ---------------------------------------------------------
         * Cube atlas UV mapping
         * The hit point is first converted into local coordinates
         * on the intersected face. The six faces are then arranged
         * into a cross-shaped texture atlas.
         ---------------------------------------------------------- */
        // Coordinates on the single face of the cube
        float u_local{0.0f}, v_local{0.0f};
        // Coordinates in the Atlas representation
        float u{0.0f}, v{0.0f};
        // Auxiliary variables for the raw coordinates
        float raw_u{0.0f}, raw_v{0.0f};
        // Variables used to position the face in the atlas cross
        float col{0.0f}, row{0.0f};

        // YZ face (+X or -X)
        if (abs_x >= abs_y && abs_x >= abs_z) {
            float s = std::copysign(1.0f, local_point.x);
            local_normal = Normal{s, 0.0f, 0.0f};

            raw_u = local_point.y * s;
            raw_v = local_point.z;

            col = 1.0f;
            row = 1.0f - s; // s=1 -> 0.0, s=-1 -> 2.0
        }
        // XZ face (+Y or -Y)
        else if (abs_y >= abs_x && abs_y >= abs_z) {
            float s = std::copysign(1.0f, local_point.y);
            local_normal = Normal{0.0f, s, 0.0f};

            raw_u = -local_point.x * s;
            raw_v = local_point.z;

            col = 1.0f - s; // s=1 -> 0.0, s=-1 -> 2.0
            row = 2.0f;
        }
        // XY face (+Z or -Z)
        else {
            float s = std::copysign(1.0f, local_point.z);
            local_normal = Normal{0.0f, 0.0f, s};

            raw_u = -local_point.y;
            raw_v = local_point.x * s;

            col = 1.0f;
            row = 2.0f + s; // s=1 -> 3.0, s=-1 -> 1.0
        }

        // Branchless normal flip based on ray direction
        float dot = local_normal * local_ray.direction;
        float flip = -std::copysign(1.0f, dot);

        local_normal = Normal{
            local_normal.x * flip, 
            local_normal.y * flip, 
            local_normal.z * flip
        };

        // Convert the local coordinates from [-1, 1] range to standard UV [0, 1] range
        u_local = (raw_u + 1.0f) * 0.5f;
        v_local = (raw_v + 1.0f) * 0.5f;

        // Multiply by reciprocal instead of dividing (faster)
        u = (col + u_local) * 0.3333333f; 
        v = (row + v_local) * 0.25f;      

        // Geometric information on the intersection point in global space
        HitRecord record;
        record.ray = ray;
        record.t = first_hit_t;
        record.hit_point = trans * local_point;
        record.hit_normal = (trans * local_normal).normalize();
        record.surface_params.u = u;
        record.surface_params.v = v;
        record.hitted_shape = this;

        return record;
    }

    [[nodiscard]] std::vector<directionalHitRecord> ray_all_intersections(const Ray& ray) const override {
        std::vector<directionalHitRecord> intersections;

        // Transform the ray into the cube's local reference frame
        Ray local_ray = ray.transform(trans.inverse());

        // Ray-box intersection using the slab method
        Vec inv_dir = Vec{
            1.0f / local_ray.direction.x,
            1.0f / local_ray.direction.y,
            1.0f / local_ray.direction.z
        };

        // ---------------------------------------------------------
        // Slab Method (Branchless)
        // ---------------------------------------------------------
        float tx1 = (-1.0f - local_ray.origin.x) * inv_dir.x;
        float tx2 = ( 1.0f - local_ray.origin.x) * inv_dir.x;
        float tmin = std::min(tx1, tx2);
        float tmax = std::max(tx1, tx2);

        float ty1 = (-1.0f - local_ray.origin.y) * inv_dir.y;
        float ty2 = ( 1.0f - local_ray.origin.y) * inv_dir.y;
        tmin = std::max(tmin, std::min(ty1, ty2));
        tmax = std::min(tmax, std::max(ty1, ty2));

        float tz1 = (-1.0f - local_ray.origin.z) * inv_dir.z;
        float tz2 = ( 1.0f - local_ray.origin.z) * inv_dir.z;
        tmin = std::max(tmin, std::min(tz1, tz2));
        tmax = std::min(tmax, std::max(tz1, tz2));

        if (tmin > tmax) return intersections;

        // Evaluate both potential hits (entry and exit)
        std::array<float, 2> candidates = {tmin, tmax};

        // Iteration on potential hits
        for (float t_curr : candidates) {
            // Check if the current t is within the valid bounds of the ray
            if (t_curr > local_ray.tmin && t_curr < local_ray.tmax) {
                
                Point local_point = local_ray.at(t_curr);
                Normal local_normal{0.0f, 0.0f, 0.0f};

                // The dominant coordinate identifies the intersected face
                float abs_x = std::abs(local_point.x);
                float abs_y = std::abs(local_point.y);
                float abs_z = std::abs(local_point.z);

                /* ---------------------------------------------------------
                 * Cube atlas UV mapping
                 * The hit point is first converted into local coordinates
                 * on the intersected face. The six faces are then arranged
                 * into a cross-shaped texture atlas.
                 ---------------------------------------------------------- */
                // Coordinates on the single face of the cube
                float u_local{0.0f}, v_local{0.0f};
                // Coordinates in the Atlas representation
                float u{0.0f}, v{0.0f};
                // Auxiliary variables for the raw coordinates
                float raw_u{0.0f}, raw_v{0.0f};
                // Variables used to position the face in the atlas cross
                float col{0.0f}, row{0.0f};

                // YZ face (+X or -X)
                if (abs_x >= abs_y && abs_x >= abs_z) {
                    float s = std::copysign(1.0f, local_point.x);
                    local_normal = Normal{s, 0.0f, 0.0f};

                    raw_u = local_point.y * s;
                    raw_v = local_point.z;

                    col = 1.0f;
                    row = 1.0f - s;
                }
                // XZ face (+Y or -Y)
                else if (abs_y >= abs_x && abs_y >= abs_z) {
                    float s = std::copysign(1.0f, local_point.y);
                    local_normal = Normal{0.0f, s, 0.0f};

                    raw_u = -local_point.x * s;
                    raw_v = local_point.z;

                    col = 1.0f - s;
                    row = 2.0f;
                }
                // XY face (+Z or -Z)
                else {
                    float s = std::copysign(1.0f, local_point.z);
                    local_normal = Normal{0.0f, 0.0f, s};

                    raw_u = -local_point.y;
                    raw_v = local_point.x * s;

                    col = 1.0f;
                    row = 2.0f + s;
                }

                // Determine entry/exit state based on the initial geometric dot product
                float dot = local_normal * local_ray.direction;
                bool is_entering = (dot < 0.0f);

                // Branchless normal flip based on ray direction
                float flip = -std::copysign(1.0f, dot);
                local_normal = Normal{
                    local_normal.x * flip, 
                    local_normal.y * flip, 
                    local_normal.z * flip
                };

                // Convert the local coordinates from [-1, 1] range to standard UV [0, 1] range
                u_local = (raw_u + 1.0f) * 0.5f;
                v_local = (raw_v + 1.0f) * 0.5f;

                // Multiply by reciprocal instead of dividing (faster)
                u = (col + u_local) * 0.3333333f; 
                v = (row + v_local) * 0.25f;      

                // Geometric information on the intersection point in global space
                directionalHitRecord record;
                record.ray = ray;
                record.t = t_curr;
                record.hit_point = trans * local_point;
                record.hit_normal = (trans * local_normal).normalize();
                record.surface_params.u = u;
                record.surface_params.v = v;
                record.hitted_shape = this;
                record.is_entering = is_entering;

                intersections.push_back(record);
            }
        }
        return intersections;
    }
};

// ============================================================================
// CYLINDER (FINITE WITH CAPS)
// ============================================================================

/**
 * @brief Finite unit cylinder centered at the local origin along the z-axis.
 * * Radius \f$r = 1.0\f$, bounded along the z-axis in \f$z \in [-1.0, 1.0]\f$.
 * * Includes flat top and bottom caps to form a closed, solid convex shape.
 */
export struct Cylinder : Shape {
    using Shape::Shape;

    /**
     * @brief Computes the closest valid intersection between the ray and the finite cylinder.
     * @param ray The incident ray in world space.
     * @return std::optional<HitRecord> The hit record if an intersection occurs, otherwise std::nullopt.
     */
    [[nodiscard]] std::optional<HitRecord> ray_intersection(const Ray& ray) const override {
        const Ray local_ray = ray.transform(trans.inverse());
        const Vec O = local_ray.origin.to_vec();
        const Vec D = local_ray.direction;

        float best_t = local_ray.tmax;
        Normal best_local_normal{0.0f, 0.0f, 0.0f};
        float best_u = 0.0f, best_v = 0.0f;

        // Intersect with the side wall: x^2 + y^2 = 1
        const float a = D.x * D.x + D.y * D.y;
        if (a > 1e-7f) {
            const float c = O.x * O.x + O.y * O.y - 1.0f;
            const float b_half = O.x * D.x + O.y * D.y;
            const float disc = b_half * b_half - a * c;

            if (disc >= 0.0f) {
                const float sqrt_disc = std::sqrt(disc);
                const float inv_a = 1.0f / a;
                const float t1 = (-b_half - sqrt_disc) * inv_a;
                const float t2 = (-b_half + sqrt_disc) * inv_a;

                for (const float t : {t1, t2}) {
                    if (t > local_ray.tmin && t < best_t) {
                        const float z = O.z + t * D.z;
                        if (z >= -1.0f && z <= 1.0f) {
                            best_t = t;
                            const Point p = local_ray.at(t);
                            
                            // Normal calculation (purely radial on XY plane)
                            Normal n{p.x, p.y, 0.0f};
                            const float len2 = n.x * n.x + n.y * n.y;
                            if (len2 > 1e-12f) {
                                const float inv_len = 1.0f / std::sqrt(len2);
                                best_local_normal = Normal{n.x * inv_len, n.y * inv_len, 0.0f};
                            } else {
                                best_local_normal = Normal{1.0f, 0.0f, 0.0f};
                            }

                            // UV Mapping for side wall
                            float u = std::atan2(p.y, p.x) / (2.0f * std::numbers::pi_v<float>);
                            best_u = (u >= 0.0f) ? u : (u + 1.0f);
                            best_v = (p.z + 1.0f) * 0.5f;
                        }
                    }
                }
            }
        }

        // Intersect with end caps (z = 1.0 and z = -1.0)
        if (std::abs(D.z) > 1e-7f) {
            // Top Cap (z = 1.0)
            const float t_top = (1.0f - O.z) / D.z;
            if (t_top > local_ray.tmin && t_top < best_t) {
                const Point p_top = local_ray.at(t_top);
                if (p_top.x * p_top.x + p_top.y * p_top.y <= 1.0f) {
                    best_t = t_top;
                    best_local_normal = Normal{0.0f, 0.0f, 1.0f};
                    best_u = (p_top.x + 1.0f) * 0.5f;
                    best_v = (p_top.y + 1.0f) * 0.5f;
                }
            }

            // Bottom Cap (z = -1.0)
            const float t_bottom = (-1.0f - O.z) / D.z;
            if (t_bottom > local_ray.tmin && t_bottom < best_t) {
                const Point p_bottom = local_ray.at(t_bottom);
                if (p_bottom.x * p_bottom.x + p_bottom.y * p_bottom.y <= 1.0f) {
                    best_t = t_bottom;
                    best_local_normal = Normal{0.0f, 0.0f, -1.0f};
                    best_u = (p_bottom.x + 1.0f) * 0.5f;
                    best_v = (p_bottom.y + 1.0f) * 0.5f;
                }
            }
        }

        // No valid hit found within range
        if (best_t >= local_ray.tmax) {
            return std::nullopt;
        }

        const Point local_point = local_ray.at(best_t);

        // Normal flipping to ensure it always faces against the incident ray
        const float dot = best_local_normal * local_ray.direction;
        const float flip = -std::copysign(1.0f, dot);
        Normal flipped_normal{best_local_normal.x * flip, best_local_normal.y * flip, best_local_normal.z * flip};

        HitRecord record;
        record.ray = ray;
        record.t = best_t;
        record.hit_point = trans * local_point;
        record.hit_normal = (trans * flipped_normal).normalize();
        record.surface_params = {best_u, best_v};
        record.hitted_shape = this;

        return record;
    }

    /**
     * @brief Computes all valid intersections between the ray and the finite cylinder (essential for CSG).
     * @param ray The incident ray in world space.
     * @return std::vector<directionalHitRecord> A collection of all sorted intersections along the ray path.
     */
    [[nodiscard]] std::vector<directionalHitRecord> ray_all_intersections(const Ray& ray) const override {
        const Ray local_ray = ray.transform(trans.inverse());
        const Vec O = local_ray.origin.to_vec();
        const Vec D = local_ray.direction;

        std::vector<directionalHitRecord> intersections;
        intersections.reserve(4);

        // Check side wall
        const float a = D.x * D.x + D.y * D.y;
        if (a > 1e-7f) {
            const float c = O.x * O.x + O.y * O.y - 1.0f;
            const float b_half = O.x * D.x + O.y * D.y;
            const float disc = b_half * b_half - a * c;

            if (disc >= 0.0f) {
                const float sqrt_disc = std::sqrt(disc);
                const float inv_a = 1.0f / a;
                const float t1 = (-b_half - sqrt_disc) * inv_a;
                const float t2 = (-b_half + sqrt_disc) * inv_a;

                for (const float t : {t1, t2}) {
                    if (t > local_ray.tmin && t < local_ray.tmax) {
                        const float z = O.z + t * D.z;
                        if (z >= -1.0f && z <= 1.0f) {
                            const Point local_point = local_ray.at(t);
                            
                            Normal n{local_point.x, local_point.y, 0.0f};
                            const float len2 = n.x * n.x + n.y * n.y;
                            if (len2 > 1e-12f) {
                                const float inv_len = 1.0f / std::sqrt(len2);
                                n = Normal{n.x * inv_len, n.y * inv_len, 0.0f};
                            } else {
                                n = Normal{1.0f, 0.0f, 0.0f};
                            }

                            float u = std::atan2(local_point.y, local_point.x) / (2.0f * std::numbers::pi_v<float>);
                            u = (u >= 0.0f) ? u : (u + 1.0f);
                            const float v = (local_point.z + 1.0f) * 0.5f;

                            // Inline Record Construction: Side Wall
                            const float dot = n * local_ray.direction;
                            const bool is_entering = (dot < 0.0f);
                            const float flip = -std::copysign(1.0f, dot);
                            Normal flipped_normal{n.x * flip, n.y * flip, 0.0f};

                            directionalHitRecord record;
                            record.ray = ray;
                            record.t = t;
                            record.hit_point = trans * local_point;
                            record.hit_normal = (trans * flipped_normal).normalize();
                            record.surface_params = {u, v};
                            record.hitted_shape = this;
                            record.is_entering = is_entering;

                            intersections.push_back(record);
                        }
                    }
                }
            }
        }

        // Check caps
        if (std::abs(D.z) > 1e-7f) {
            // Top Cap (z = 1.0)
            const float t_top = (1.0f - O.z) / D.z;
            if (t_top > local_ray.tmin && t_top < local_ray.tmax) {
                const Point p_top = local_ray.at(t_top);
                if (p_top.x * p_top.x + p_top.y * p_top.y <= 1.0f) {
                    const float u = (p_top.x + 1.0f) * 0.5f;
                    const float v = (p_top.y + 1.0f) * 0.5f;

                    Normal n_top{0.0f, 0.0f, 1.0f};
                    const float dot = n_top * local_ray.direction;
                    const bool is_entering = (dot < 0.0f);
                    const float flip = -std::copysign(1.0f, dot);
                    Normal flipped_normal{0.0f, 0.0f, 1.0f * flip};

                    directionalHitRecord record;
                    record.ray = ray;
                    record.t = t_top;
                    record.hit_point = trans * p_top;
                    record.hit_normal = (trans * flipped_normal).normalize();
                    record.surface_params = {u, v};
                    record.hitted_shape = this;
                    record.is_entering = is_entering;

                    intersections.push_back(record);
                }
            }

            // Bottom Cap (z = -1.0)
            const float t_bottom = (-1.0f - O.z) / D.z;
            if (t_bottom > local_ray.tmin && t_bottom < local_ray.tmax) {
                const Point p_bottom = local_ray.at(t_bottom);
                if (p_bottom.x * p_bottom.x + p_bottom.y * p_bottom.y <= 1.0f) {
                    const float u = (p_bottom.x + 1.0f) * 0.5f;
                    const float v = (p_bottom.y + 1.0f) * 0.5f;

                    Normal n_bottom{0.0f, 0.0f, -1.0f};
                    const float dot = n_bottom * local_ray.direction;
                    const bool is_entering = (dot < 0.0f);
                    const float flip = -std::copysign(1.0f, dot);
                    Normal flipped_normal{0.0f, 0.0f, -1.0f * flip};

                    directionalHitRecord record;
                    record.ray = ray;
                    record.t = t_bottom;
                    record.hit_point = trans * p_bottom;
                    record.hit_normal = (trans * flipped_normal).normalize();
                    record.surface_params = {u, v};
                    record.hitted_shape = this;
                    record.is_entering = is_entering;

                    intersections.push_back(record);
                }
            }
        }

        return intersections;
    }
};


// ===================================================================================
// WORLD STRUCT
// ===================================================================================

/**
 * @brief Collection of shapes forming a scene.
 *
 * The World is responsible for finding the closest intersection between a ray and the scene geometry.
 */
export struct World {

    /// Shapes contained in the scene.
    std::vector<std::unique_ptr<Shape>> shapes;

    /**
     * @brief Add a shape to the scene.
     *
     * Ownership is transferred to the World.
     */
    void add(std::unique_ptr<Shape> shape) {
        shapes.push_back(std::move(shape));
    }

    /**
     * @brief Find the closest intersection along a ray.
     *
     * @param ray Ray to test.
     * @return Closest valid intersection, if any.
     */
    [[nodiscard]] std::optional<HitRecord> ray_intersection(Ray ray) const {
        // Default: no hit
        std::optional<HitRecord> closest = std::nullopt;

        // Cycle on shapes in the scene
        for (const auto& shape : shapes) {
            if (auto intersection = shape->ray_intersection(ray)){
                closest = intersection;

                // Update the maximum ray distance so that only closer intersections can be accepted.
                ray.tmax = closest->t;
            }
        }
        return closest;
    }
};

