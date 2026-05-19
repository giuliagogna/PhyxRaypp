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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import std;
import Color;
import Geometry;
import auxiliary_functions;
import Pigment;
import BRDF;
import PCG;
import Camera;

TEST_CASE("Test BRDF constructors") {

    UniformPigment white{Color{1.0f, 1.0f, 1.0f}};

    SUBCASE("DiffusiveBRDF") {
        DiffusiveBRDF diffusive(std::make_shared<UniformPigment>(white));
        CHECK(diffusive.pigment->get_color(Vec2D{0.0f, 0.0f}).is_close(white.get_color(Vec2D{0.0f, 0.0f})));
    }

    SUBCASE("SpecularBRDF") {
        SpecularBRDF specular(std::make_shared<UniformPigment>(white));

        CHECK(specular.pigment->get_color(Vec2D{0.0f, 0.0f}).is_close(white.get_color(Vec2D{0.0f, 0.0f})));
        CHECK(aux::are_close(specular.threshold_angle_rad, std::numbers::pi_v<float> / 1800.0f));
    }
}

TEST_CASE("Test scatter_ray method") {
    PCG pcg;
    UniformPigment blue(Color{0.0f, 0.0f, 1.0f});
    Point hit_point{0.0f, 0.0f, 0.0f};
    Normal normal{0.0f, 0.0f, 1.0f};
    int depth = 2;

    SUBCASE("SpecularBRDF deterministic reflection") {
        SpecularBRDF specular; // Uses default white pigment

        // Ray coming in at a 45-degree angle (down and to the right)
        Vec incoming_dir = Vec{1.0f, 0.0f, -1.0f}.normalize();

        Ray scattered = specular.scatter_ray(pcg, incoming_dir, hit_point, normal, depth);

        // A perfect mirror should reflect it 45 degrees (up and to the right)
        Vec expected_out_dir = Vec{1.0f, 0.0f, 1.0f}.normalize();

        // Check if the ray properties are correct
        CHECK(scattered.origin.is_close(hit_point));
        CHECK(scattered.direction.is_close(expected_out_dir));
        CHECK(scattered.depth == depth);
        CHECK(scattered.tmin == 1.0e-3f);
    }

    SUBCASE("DiffusiveBRDF stochastic reflection properties") {
        DiffusiveBRDF diffuse; // Uses default white pigment

        // Ray coming straight down
        Vec incoming_dir = Vec{0.0f, 0.0f, -1.0f};

        Ray scattered = diffuse.scatter_ray(pcg, incoming_dir, hit_point, normal, depth);

        // Check basic ray properties
        CHECK(scattered.origin.is_close(hit_point));
        CHECK(scattered.depth == depth);
        CHECK(scattered.tmin == 1.0e-3f);

        // Check that the direction vector is normalized (length == 1)
        CHECK(aux::are_close(scattered.direction.norm(), 1.0f));

        // Hemisphere check: The dot product of the outgoing ray and the normal
        // MUST be >= 0. If it's negative, the ray bounced INSIDE the object!
        float dot_product = scattered.direction * normal;
        CHECK(dot_product >= 0.0f);
    }
}