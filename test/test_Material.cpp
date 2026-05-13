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
import Material;

TEST_CASE("Test Material constructors") {

    SUBCASE("Default constructor provides non-emitting white material") {
        Material default_mat; // Uses the default arguments

        // Verify pointers are not null
        REQUIRE(default_mat.brdf != nullptr);
        REQUIRE(default_mat.emitted_radiance != nullptr);

        // Verify the default emission is pure black (doesn't glow)
        Vec2D dummy_uv{0.0f, 0.0f};
        Color expected_emission{0.0f, 0.0f, 0.0f};
        CHECK(default_mat.emitted_radiance->get_color(dummy_uv).is_close(expected_emission));
    }

    SUBCASE("Custom constructor correctly assigns pointers") {
        // Build concrete components
        auto red_pigment = std::make_shared<UniformPigment>(Color{1.0f, 0.0f, 0.0f});
        auto custom_brdf = std::make_shared<DiffusiveBRDF>(red_pigment);

        auto green_emission = std::make_shared<UniformPigment>(Color{0.0f, 1.0f, 0.0f});

        // Pass them into the material
        Material custom_mat(custom_brdf, green_emission);

        // Verify they were safely moved and stored
        REQUIRE(custom_mat.brdf != nullptr);
        REQUIRE(custom_mat.emitted_radiance != nullptr);

        // Verify the emission is the exact green we passed in
        Vec2D dummy_uv{0.0f, 0.0f};
        Color expected_emission{0.0f, 1.0f, 0.0f};
        CHECK(custom_mat.emitted_radiance->get_color(dummy_uv).is_close(expected_emission));
    }
}