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
 * @file CSG.cppm
 * @brief Constructive Solid Geometry operations for combining shapes.
 *
 * This module defines the CSG operations that can be used to combine different geometric primitives.
 */

module;

export module CSG;
import std;
import Geometry;
import Camera;
import auxiliary_functions;
import Color;
import HDRImage;
import Material;
import Pigment;
import BRDF;
import Shape;

export enum class CSGOperations {
    /// Combines two shapes into a single shape that encompasses both.
    Union,        

    /// Creates a shape that represents the overlapping volume of two shapes.
    Intersection, 

    /// Subtracts one shape from another, resulting in a shape that 
    /// represents the volume of the first shape minus the second.
    Difference    
};
/*
@brief Constructive Solid Geometry (CSG) shape that combines two shapes using a specified operation.
 *
 * The CSG shape allows for the creation of complex geometries by performing boolean operations
 * on simpler shapes. It supports union, intersection, and difference operations.
 * 
 * The fact that CSG is derived from Shape means it can be used anywhere a Shape is expected, allowing for nested CSG operations.
*/
export struct CSG : Shape {
    std::unique_ptr<Shape> left;  // Left operand shape.
    std::unique_ptr<Shape> right; // Right operand shape.
    CSGOperations operation;      // CSG operation to apply.

    /**
     * @brief Contructor for the CSG shape.
     *
     * It initializes the CSG shape with two operand shapes and a specified operation.
     * 
     *
     * @param left Pointer to the left operand shape.
     * @param right Pointer to the right operand shape.
     * @param operation CSG operation to apply (Union, Intersection, Difference). Difference is defined as left - right.
     */
    CSG(std::unique_ptr<Shape> left, std::unique_ptr<Shape> right, CSGOperations operation)
        : left(std::move(left)), right(std::move(right)), operation(operation) {}

    /**
     * @brief Performs a ray intersection test with the CSG shape, recursively checking the left and right operand shapes.
     *
     * @param ray The ray to test for intersection.
     * @return An optional HitRecord if an intersection is found, otherwise std::nullopt.
     */
    [[nodiscard]] std::optional<HitRecord> ray_intersection(const Ray& ray) const override {
        // Since ray_all_intersections already computes, sorts, and filters all valid hits
        // in the correct world bounds, we just need to return the first one (if any exists).
        auto all_hits = ray_all_intersections(ray);
        
        if (!all_hits.empty()) {
            return static_cast<HitRecord>(all_hits.front());
        }
        return std::nullopt;
    }

    /**
     * @brief Finds all intersections of a ray with the CSG shape, recursively checking the left and right operand shapes.
     * * This allows for the collection of all intersection points, which is useful for CSG operations that require knowledge of all entry and exit points.
     *
     * @param ray The ray to test for intersections.
     * @return A vector of all found intersection records.
     */
    [[nodiscard]] std::vector<directionalHitRecord> ray_all_intersections(const Ray& ray) const override {
        std::vector<directionalHitRecord> valid_intersections;

        if (!left || !right) return valid_intersections;

        Ray local_ray = ray.transform(trans.inverse());

        // Possible recursion if left and right shapes are CSG nodes themselves
        auto left_hits = left->ray_all_intersections(local_ray);
        auto right_hits = right->ray_all_intersections(local_ray);

        // Sorting both hit lists by the t value
        std::ranges::sort(left_hits, {}, &directionalHitRecord::t);
        std::ranges::sort(right_hits, {}, &directionalHitRecord::t);

        // Deduction of initial state, inside or outside the left and right shape at the start of the ray
        bool in_left = !left_hits.empty() && !left_hits.front().is_entering;
        bool in_right = !right_hits.empty() && !right_hits.front().is_entering;

        // Reserving space for the valid intersections to avoid multiple reallocations
        valid_intersections.reserve(left_hits.size() + right_hits.size());

        // Iterators for traversing the left and right hit lists
        auto it_L = left_hits.begin();
        auto it_R = right_hits.begin();

        // Managing the merging of the two sorted hit lists and applying the CSG operation
        while (it_L != left_hits.end() || it_R != right_hits.end()) {
            
            // Check which hit to process next based on the t value, ensuring we always process the closest hit first
            bool process_left;
            if (it_L == left_hits.end()) process_left = false; // No more left hits, process right
            else if (it_R == right_hits.end()) process_left = true; // No more right hits, process left
            else process_left = (it_L->t < it_R->t); // Compare t values to decide which hit to process next

            directionalHitRecord hit; // A copy of the hit record to be potentially added to the valid intersections list
            bool keep_hit = false;

            if (process_left) {
                hit = std::move(*it_L); // Move the left hit to avoid unnecessary copying
                in_left = hit.is_entering; // Update the state of being inside or outside the left shape based on the hit's entering/exiting status
                
                // Evaluation of the utility of such hit based on the CSG operation and the current state of being inside or outside the right shape
                switch (operation) {
                    // In union, we keep the hit if we are not inside the right shape (no actual surface intersection)
                    case CSGOperations::Union:        keep_hit = !in_right; break;
                    // In intersection, we keep the hit if we are inside the right shape. Otherwise, we are outside the intersection volume.
                    case CSGOperations::Intersection: keep_hit = in_right; break;
                    // In difference, we keep the hit if we are outside the right shape. Otherwise, we are inside the volume that is being subtracted.
                    case CSGOperations::Difference:   keep_hit = !in_right; break;
                }
                ++it_L; // Update iterator for the left hits to move to the next hit in the list
            } else { // process right hit
                hit = std::move(*it_R);
                in_right = hit.is_entering; // Update the state of being inside or outside the right shape based on the hit's entering/exiting status
                
                // Evaluation of the utility of such hit based on the CSG operation and the current state of being inside or outside the left shape
                switch (operation) {
                    // In union, we keep the hit if we are not inside the left shape (no actual surface intersection)
                    case CSGOperations::Union:        keep_hit = !in_left; break;
                    // In intersection, we keep the hit if we are inside the left shape. Otherwise, we are outside the intersection volume.
                    case CSGOperations::Intersection: keep_hit = in_left; break;
                    // In difference, we check if we are inside the left space. If we are, we keep the hit but we need to flip the normal and the entering/exiting status to reflect the subtraction operation.
                    case CSGOperations::Difference: 
                        keep_hit = in_left;
                        if (keep_hit) {
                            hit.hit_normal = -hit.hit_normal;
                            hit.is_entering = !hit.is_entering;
                        } // It's now an outcoming hit from the modified left shape
                        break;
                }
                ++it_R; // Update iterator for the right hits to move to the next hit in the list
            }

            // Transformation and saving only if the hit is valid
            if (keep_hit) {
                hit.hit_point = trans * hit.hit_point;
                hit.hit_normal = (trans * hit.hit_normal).normalize();
                hit.ray = ray;
                valid_intersections.push_back(std::move(hit));
            }
        }

        // Returns shrinked, readapted and transformed valid intersections in world coordinates
        return valid_intersections;
    }
};