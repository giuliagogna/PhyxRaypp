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

import Color;
import Pigment;
import Geometry;
import HDRImage;

// ===========================================================
// TEST 1: Uniform Pigment
// ===========================================================
TEST_CASE("TEST 1: Uniform Pigment") {
    Color color{1.0f, 0.0f, 0.0f};
    UniformPigment pigment(color);

    CHECK(pigment.get_color(Vec2D{0.4f, 0.2f}).is_close(color));
    CHECK(pigment.get_color(Vec2D{0.6f, 0.8f}).is_close(color));
}

// ===========================================================
// TEST 2: Checkered Pigment
// ===========================================================
TEST_CASE("TEST 2: Checkered Pigment") {

    Color color1{1.0f, 0.0f, 0.0f};
    Color color2{0.0f, 1.0f, 0.0f};
    int n_steps = 10;
    CheckeredPigment pigment(color1, color2, n_steps);

    SUBCASE("(u, v) both odd") {
        Vec2D surf_par{0.5f, 0.7f}; // both odd
        CHECK(pigment.get_color(surf_par).is_close(color1));
    }

    SUBCASE("(u, v) both even") {
        Vec2D surf_par{0.437f, 0.895f}; // both even
        CHECK(pigment.get_color(surf_par).is_close(color1));
    }

    SUBCASE("u even and v odd") {
        Vec2D surf_par{0.4f, 0.9f}; // u even and v odd
        CHECK(pigment.get_color(surf_par).is_close(color2));
    }

    SUBCASE("u odd and v even") {
        Vec2D surf_par{0.52f, 0.649f}; // u odd and v even
        CHECK(pigment.get_color(surf_par).is_close(color2));
    }

    SUBCASE("Edge case: Negative coordinates") {
        // u * 10 = -1.5 -> floor is -2 (even)
        // v * 10 = -2.5 -> floor is -3 (odd)
        Vec2D surf_par{-0.15f, -0.25f};
        CHECK(pigment.get_color(surf_par).is_close(color2));
    }

    SUBCASE("Edge case: Origin") {
        // u=0, v=0 -> both are 0 (even)
        Vec2D surf_par_zero{0.0f, 0.0f};
        CHECK(pigment.get_color(surf_par_zero).is_close(color1));
    }

    SUBCASE("Coordinates bigger than 1") {
        // u * 10 = 21.5 -> floor is 21
        // v * 10 = 42.5 -> floor is 42
        Vec2D surf_par{2.15f, 4.25f};
        CHECK(pigment.get_color(surf_par).is_close(color2));
    }
}

// ===========================================================
// TEST 3: Image Pigment
// ===========================================================
TEST_CASE("TEST 3: Image Pigment") {
    HDRImage img(2, 2);

    Color red{1.0f, 0.0f, 0.0f};
    Color green{0.0f, 1.0f, 0.0f};
    Color blue{0.0f, 0.0f, 1.0f};
    Color white{1.0f, 1.0f, 1.0f};

    // Top-left is Red, Top-right is Green
    img.set_pixel(0, 0, red);
    img.set_pixel(1, 0, green);
    // Bottom-left is Blue, Bottom-right is White
    img.set_pixel(0, 1, blue);
    img.set_pixel(1, 1, white);

    ImagePigment pigment(img);

    SUBCASE("Interior pixel mapping") {
        // u, v = 0.25 -> 0.25 * 2 = 0.5 -> floor(0.5) = col/row 0 -> Top-Left
        CHECK(pigment.get_color(Vec2D{0.25f, 0.25f}).is_close(red));

        // u = 0.75, v = 0.25 -> col 1, row 0 -> Top-Right
        CHECK(pigment.get_color(Vec2D{0.75f, 0.25f}).is_close(green));

        // u = 0.25, v = 0.75 -> col 0, row 1 -> Bottom-Left
        CHECK(pigment.get_color(Vec2D{0.25f, 0.75f}).is_close(blue));

        // u, v = 0.75 -> col 1, row 1 -> Bottom-Right
        CHECK(pigment.get_color(Vec2D{0.75f, 0.75f}).is_close(white));
    }

    SUBCASE("Exact boundaries (0.0 and 1.0)") {
        // Exactly 0.0 should hit the very first pixel
        CHECK(pigment.get_color(Vec2D{0.0f, 0.0f}).is_close(red));

        // Exactly 1.0 should trigger the clamping and hit the very last pixel
        CHECK(pigment.get_color(Vec2D{1.0f, 1.0f}).is_close(white));

        // Mixed boundaries
        CHECK(pigment.get_color(Vec2D{1.0f, 0.0f}).is_close(green));
    }

    SUBCASE("Out of bounds clamping (Negative and > 1.0)") {
        // Negative coordinates should clamp to 0 (Top-Left)
        CHECK(pigment.get_color(Vec2D{-0.5f, -2.0f}).is_close(red));

        // Coordinates greater than 1.0 should clamp to max width/height (Bottom-Right)
        CHECK(pigment.get_color(Vec2D{1.5f, 3.14f}).is_close(white));
    }
}
