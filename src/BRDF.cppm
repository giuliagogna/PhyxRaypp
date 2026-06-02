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

module;

export module BRDF;
import std;
import Color;
import Geometry;
import Camera;
import Pigment;
import PCG;

// Virtual DRDF class
export struct BRDF {
    std::shared_ptr<Pigment> pigment;

    BRDF(std::shared_ptr<Pigment> pigment) : pigment(pigment) {}

    virtual Ray scatter_ray(
        PCG& pcg,
        Vec incoming_direction,
        Point interaction_point,
        Normal normal,
        int depth
        ) = 0;
};

// Pure isotropic diffusion BRDF. Avoiding trigonometric functions to improve performace
export struct DiffusiveBRDF : BRDF {
    DiffusiveBRDF(std::shared_ptr<Pigment> pigment = std::make_shared<UniformPigment>(Color{1.0f, 1.0f, 1.0f})) :
        BRDF(std::move(pigment)) {}

    Ray scatter_ray(PCG& pcg, Vec incoming_direction, Point interaction_point, Normal normal, int depth) override {
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
    }
};

// Specular reflection BRDF, chosen sharpness
export struct SpecularBRDF : BRDF {
    float threshold_angle_rad;

    SpecularBRDF(std::shared_ptr<Pigment> pigment = std::make_shared<UniformPigment>(Color{1.0f, 1.0f, 1.0f}),
                 float threshold_angle = std::numbers::pi_v<float> / 1800.0f) :
        BRDF(std::move(pigment)), threshold_angle_rad(threshold_angle) {}

    Ray scatter_ray(PCG& pcg, Vec incoming_direction, Point interaction_point, Normal normal, int depth) override {
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


