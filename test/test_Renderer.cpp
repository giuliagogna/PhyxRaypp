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
import Pigment;
import BRDF;
import Material;
import Camera;
import Renderer;
import Shape;

// =========================================================================
// TEST: Renderers
// =========================================================================
TEST_CASE("TEST: Renderers (OnOff and Flat)") {
    World world;

    // Create sphere with a green material
    auto green_pigment = std::make_shared<UniformPigment>(Color{0.0f, 1.0f, 0.0f});
    auto brdf = std::make_shared<DiffusiveBRDF>(green_pigment);
    auto material = std::make_shared<Material>(brdf);

    // Sphere is at the origin (0,0,0) by default
    world.add(std::make_unique<Sphere>(Transformation{}, material));

    // Create a second sphere with no material to test the fallback
    world.add(std::make_unique<Sphere>(Trans(Vec{5.0f, 0.0f, 0.0f}), nullptr));

    // Test rays
    Ray hit_ray{Point{0.0f, 0.0f, 2.0f}, Vec{0.0f, 0.0f, -1.0f}};      // Hits the Green sphere
    Ray miss_ray{Point{0.0f, 0.0f, 2.0f}, Vec{0.0f, 1.0f, 0.0f}};      // Shoots up, misses everything
    Ray fallback_ray{Point{5.0f, 0.0f, 2.0f}, Vec{0.0f, 0.0f, -1.0f}}; // Hits the material-less sphere

    Color sky_blue{0.5f, 0.7f, 1.0f};

    SUBCASE("OnOffRenderer logic") {
        // Setup: Sky is Blue, Hit Color is White
        OnOffRenderer renderer(&world, sky_blue, Color{1.0f, 1.0f, 1.0f});

        // Hitting the sphere should return White (ignores the material color)
        CHECK(renderer(hit_ray).is_close(Color{1.0f, 1.0f, 1.0f}));

        // Missing the sphere should return Sky Blue
        CHECK(renderer(miss_ray).is_close(sky_blue));
    }

    SUBCASE("FlatRenderer logic") {
        FlatRenderer renderer(&world, sky_blue);

        // Hitting the sphere should return the exact Green pigment from the material
        CHECK(renderer(hit_ray).is_close(Color{0.0f, 1.0f, 0.0f}));

        // Missing the sphere should return Sky Blue
        CHECK(renderer(miss_ray).is_close(sky_blue));

        // Hitting a shape with NO material should trigger the Red fallback
        CHECK(renderer(fallback_ray).is_close(Color{1.0f, 0.0f, 0.0f}));
    }
}