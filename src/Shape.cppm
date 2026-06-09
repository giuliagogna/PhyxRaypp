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
     * @brief Compute the intersection between a ray and the shape.
     *
     * @param ray Ray to test.
     * @return HitRecord object if a hit occurs, nullptr otherwise.
     */
    virtual std::optional<HitRecord> ray_intersection(const Ray& ray) const = 0; // Pure virtual method to compute ray-shape intersection
    
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

    /// Returns a HitRecord in the axis origin frame if the ray intersects the sphere, std::nullopt otherwise
    std::optional<HitRecord> ray_intersection(const Ray& ray) const { // Override method to compute ray-sphere intersection

        // Transform the ray to the sphere reference frame (where the sphere is a unit sphere centered at the origin)
        Ray local_ray = ray.transform(trans.inverse());

        Vec O = local_ray.origin.to_vec(); // ray origin is stored as a Vec, so it can be normalized
                                           // (under the carpet the calculation is Point ray_origin - Point axes_origin = Vec ray_origin)
        Vec D = local_ray.direction;

        float dir2 = D.norm2();

        float b_half = O * D;
        float discriminant = (b_half * b_half) - dir2 * (O.norm2() - 1.0f);

        if (discriminant < 0.0f) {
            return std::nullopt;
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

        // Since the calculations assume to have a unit sphere centered at the origin the coordinates of the point of
        // intersection are exactly the components of the normal to the sphere in that point
        Normal local_normal{local_point.x, local_point.y, local_point.z};
        if (local_point.to_vec() * D > 0.0f) {
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

};

// ==================================
// PLANE
// ==================================

/**
 * @brief Infinite plane lying on the local z = 0 plane.
 */
export struct Plane : Shape {

    using Shape::Shape; // Inherits constructors from Shape (allows passing Transformation)

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

        // Return geometric information for intersection in global space
        Point local_point = local_ray.at(t);
        HitRecord record;
        record.ray = ray;
        record.t = t;
        record.hit_point = trans * local_point;
        Normal local_normal = (local_ray.direction.z < 0) ? Normal{0.0f, 0.0f, 1.0f} : Normal{0.0f, 0.0f, -1.0f};
        record.hit_normal = trans * local_normal;

        // Generate repeating texture coordinates from local position.
        record.surface_params.u = local_point.x - std::floor(local_point.x);
        record.surface_params.v = local_point.y - std::floor(local_point.y);
        record.hitted_shape = this;

        return record;
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

        // Transform the ray into the cube's local reference frame.
        Ray local_ray = ray.transform(trans.inverse());

        // Ray-box intersection using the slab method.
        Vec inv_direction = Vec{
                                1.0f/local_ray.direction.x,
                                1.0f/local_ray.direction.y,
                                1.0f/local_ray.direction.z
                                };

        // left-bottom-behind corner and right-upper-front corner of the canonical cube
        Vec bounds[2] = {Vec{-1.0f, -1.0f, -1.0f}, Vec{1.0f, 1.0f, 1.0f}};

        // Determine which side of each slab is encountered first.
        int sign[3];
        sign[0] = (local_ray.direction.x < 0) ? 1 : 0;
        sign[1] = (local_ray.direction.y < 0) ? 1 : 0;
        sign[2] = (local_ray.direction.z < 0) ? 1 : 0;

        float tmin = (bounds[sign[0]].x - local_ray.origin.x) * inv_direction.x;
        float tmax = (bounds[1-sign[0]].x - local_ray.origin.x) * inv_direction.x;
        float tymin = (bounds[sign[1]].y - local_ray.origin.y) * inv_direction.y;
        float tymax = (bounds[1-sign[1]].y - local_ray.origin.y) * inv_direction.y;

        if (tmin > tymax || tymin > tmax) return std::nullopt;
        if (tymin > tmin) tmin = tymin;
        if (tymax < tmax) tmax = tymax;

        float tzmin = (bounds[sign[2]].z - local_ray.origin.z) * inv_direction.z;
        float tzmax = (bounds[1-sign[2]].z - local_ray.origin.z) * inv_direction.z;

        if (tmin > tzmax || tzmin > tmax) return std::nullopt;
        if (tzmin > tmin) tmin = tzmin;
        if (tzmax < tmax) tmax = tzmax;

        // Choose the first valid intersection along the ray.
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

        // The dominant coordinate identifies the intersected face.
        float abs_x = std::abs(local_point.x);
        float abs_y = std::abs(local_point.y);
        float abs_z = std::abs(local_point.z);

        if (abs_x >= abs_y && abs_x >= abs_z) {
            local_normal = Normal{(local_point.x > 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f};
        } else if (abs_y >= abs_x && abs_y >= abs_z) {
            local_normal = Normal{0.0f, (local_point.y > 0.0f) ? 1.0f : -1.0f, 0.0f};
        } else {
            local_normal = Normal{0.0f, 0.0f, (local_point.z > 0.0f) ? 1.0f : -1.0f};
        }

        // Flip the normal for rays originating inside the cube.
        if (local_point.to_vec() * local_ray.direction > 0.0f) {
            local_normal = -local_normal;
        }

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
        if (std::abs(local_normal.x) > 0.5f) {
            // Compute local face coordinates with a consistent orientation.
            raw_u = (local_point.x > 0.0f) ? local_point.y : -local_point.y;
            raw_v = local_point.z;

            // Set the position in the cross
            col = 1.0f;
            row = (local_point.x > 0.0f) ? 0.0f : 2.0f;
        }
        // XZ face (+Y or -Y)
        else if (std::abs(local_normal.y) > 0.5f) {
            raw_u = (local_point.y > 0.0f) ? -local_point.x : local_point.x;
            raw_v = local_point.z;

            // Set the position in the cross
            col = (local_point.y > 0.0f) ? 0.0f : 2.0f;
            row = 2.0f;
        }
        // XY face (+Z or -Z)
        else if (std::abs(local_normal.z) > 0.5f) {
            // Top (+Z) and bottom (-Z) faces.
            //
            // UV coordinates are chosen so that neighbouring faces share a consistent orientation when
            // unfolded into the cube-map cross atlas.
            raw_u = -local_point.y;
            raw_v = (local_point.z > 0.0f) ? local_point.x : -local_point.x;

            // Set the position in the cross
            col = 1.0f;
            row = (local_point.z > 0.0f) ? 3.0f : 1.0f;
        }

        // Convert the local coordinates from [-1, 1] range to standard UV [0, 1] range
        u_local = (raw_u + 1.0f) * 0.5f;
        v_local = (raw_v + 1.0f) * 0.5f;

        u = (col + u_local)/3.0f;
        v = (row + v_local)/4.0f;


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

