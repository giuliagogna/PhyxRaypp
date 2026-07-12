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
 * @file BRDF.cppm
 * @brief Bidirectional Reflectance Distribution Functions.
 *
 * This module defines the surface scattering models used by the renderer.
 *
 * Each BRDF determines how an incoming ray is scattered after interacting
 * with a surface.
 */

module;

export module BRDF;
import std;
import Color;
import Geometry;
import Camera;
import Pigment;
import PCG;

/**
 * @brief Base class for surface scattering models.
 *
 * A BRDF determines the direction of rays scattered by a surface after an interaction. The associated pigment
 * defines the surface color or reflectance.
 */
export struct BRDF {

    /// Pigment controlling the surface reflectance.
    std::shared_ptr<Pigment> pigment;

    /// Construct a BRDF with the given pigment.
    BRDF(std::shared_ptr<Pigment> pigment) : pigment(pigment) {}

    /**
     * @brief Generate a scattered ray.
     *
     * Computes a new ray originating from the interaction point according to the scattering model implemented by the BRDF.
     *
     * @param pcg Random number generator used for sampling.
     * @param incoming_direction Direction of the incoming ray.
     * @param interaction_point Surface interaction point.
     * @param normal Surface normal at the interaction point.
     * @param depth Recursion depth carried by the generated ray.
     *
     * @return Scattered ray.
     */
    virtual Ray scatter_ray(
        PCG& pcg,
        Vec incoming_direction,
        Point interaction_point,
        Normal normal,
        int depth
        ) = 0;

    /// Virtual destructor.
    virtual ~BRDF() = default;
};

/**
 * @brief Ideal diffuse BRDF.
 *
 * Scatters rays uniformly over the hemisphere centered on the surface normal.
 */
export struct DiffusiveBRDF : BRDF {

    /**
     * @brief Construct a diffusive BRDF.
     *
     * By default, creates a perfectly diffusive white material.
     */
    DiffusiveBRDF(std::shared_ptr<Pigment> pigment = std::make_shared<UniformPigment>(Color{1.0f, 1.0f, 1.0f})) :
        BRDF(std::move(pigment)) {}

    Ray scatter_ray(PCG& pcg, Vec incoming_direction, Point interaction_point, Normal normal, int depth) override {

        // Local orthonormal basis aligned with the surface normal.
        auto [e1, e2, e3] = create_onb_from_z(normal);

        float x, y, r_sq;
        do {
            x = 2.0f * pcg.random_float() - 1.0f;
            y = 2.0f * pcg.random_float() - 1.0f;
            r_sq = x * x + y * y;
        } while (r_sq >= 1.0f);

        float z = std::sqrt(1.0f - r_sq);

        return Ray{
            interaction_point,
            e1 * x + e2 * y + e3 * z,
            1.0e-3f,
            std::numeric_limits<float>::infinity(),
            depth
        };
    };
};

/**
 * @brief Ideal specular reflection BRDF.
 *
 * Scatters rays according to the law of reflection.
 */
export struct SpecularBRDF : BRDF {

    /**
     * @brief Ideal mirror BRDF.
     *
     * Scatters rays according to the law of reflection.
     * The reflected direction is deterministic and does not require random sampling.
     */
    SpecularBRDF(std::shared_ptr<Pigment> pigment = std::make_shared<UniformPigment>(Color{1.0f, 1.0f, 1.0f})) :
        BRDF(std::move(pigment)) {}

    Ray scatter_ray(PCG& pcg, Vec incoming_direction, Point interaction_point, Normal normal, int depth) override {
        // Compute the mirror-reflected direction.
        Vec ray_dir = incoming_direction.normalize();
        Vec normal_vec = Vec{normal.x, normal.y, normal.z}.normalize();
        float dot_product = normal_vec * ray_dir;

        return Ray{
            interaction_point,
            ray_dir - normal_vec * 2 * dot_product,
            1.0e-3f,
            std::numeric_limits<float>::infinity(),
            depth
        };
    };
};


