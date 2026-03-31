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
import auxiliary_functions;
import Color;

// =============================================================================
// Testing algebraic operations on Color
// =============================================================================

TEST_CASE("Testing algebraic operations on Color")
{
    Color c1{0.2f, 0.4f, 0.6f};
    Color c2{0.1f, 0.3f, 0.5f};
    float scalar = 2.0f;

    // Test addition

    Color c3 = c1;
    c3 += c2;
    CHECK(aux::are_close(c1.r + c2.r, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g + c2.g, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b + c2.b, c3.b, 1e-5f));

    c3 = c1 + c2;
    CHECK(aux::are_close(c1.r + c2.r, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g + c2.g, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b + c2.b, c3.b, 1e-5f));

    // Test subtraction
    
    c3 = c1;
    c3 -= c2;
    CHECK(aux::are_close(c1.r - c2.r, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g - c2.g, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b - c2.b, c3.b, 1e-5f));

    c3 = c1 - c2;
    CHECK(aux::are_close(c1.r - c2.r, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g - c2.g, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b - c2.b, c3.b, 1e-5f));

    // Test multiplication
    
    c3 = c1;
    c3 *= c2;
    CHECK(aux::are_close(c1.r * c2.r, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g * c2.g, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b * c2.b, c3.b, 1e-5f));

    c3 = c1 * c2;
    CHECK(aux::are_close(c1.r * c2.r, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g * c2.g, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b * c2.b, c3.b, 1e-5f));

    // Test scalar multiplication
    
    c3 = c1;
    c3 *= scalar;
    CHECK(aux::are_close(c1.r * scalar, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g * scalar, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b * scalar, c3.b, 1e-5f));

    c3 = c1 * scalar;
    CHECK(aux::are_close(c1.r * scalar, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g * scalar, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b * scalar, c3.b, 1e-5f));

    c3 = scalar * c1;
    CHECK(aux::are_close(scalar * c1.r, c3.r, 1e-5f));
    CHECK(aux::are_close(scalar * c1.g, c3.g, 1e-5f));
    CHECK(aux::are_close(scalar * c1.b, c3.b, 1e-5f));

    // Test division
    
    c3 = c1;
    c3 /= c2;
    CHECK(aux::are_close(c1.r / c2.r, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g / c2.g, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b / c2.b, c3.b, 1e-5f));

    c3 = c1 / c2;
    CHECK(aux::are_close(c1.r / c2.r, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g / c2.g, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b / c2.b, c3.b, 1e-5f));

    // Test division by scalar

    c3 = c1;
    c3 /= scalar;
    CHECK(aux::are_close(c1.r / scalar, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g / scalar, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b / scalar, c3.b, 1e-5f));

    c3 = c1 / scalar;
    CHECK(aux::are_close(c1.r / scalar, c3.r, 1e-5f));
    CHECK(aux::are_close(c1.g / scalar, c3.g, 1e-5f));
    CHECK(aux::are_close(c1.b / scalar, c3.b, 1e-5f));

}

// =============================================================================
// Color::are_close() method
// =============================================================================

TEST_CASE("Testing Color::is_close() method")
{
    Color c1{0.2f, 0.4f, 0.6f};
    Color c2{0.2f, 0.4f, 0.6f};
    Color c3{0.20001f, 0.40001f, 0.60001f};
    Color c4{0.21f, 0.41f, 0.61f};

    CHECK(c1.is_close(c2, 1e-5f) == true);   // Identical colors
    CHECK(c1.is_close(c3, 1e-5f) == true);   // Colors close within tolerance
    CHECK(c1.is_close(c4, 1e-5f) == false);  // Colors not close within tolerance

}

// =============================================================================
// Testing Color::format 
// =============================================================================

TEST_CASE("Testing Color formatting (std::formatter)")
{   
    // choosing values to test rounding
    Color c{0.2135f, 0.45751f, 0.6144f};
    
    // straight-forward case
    std::string expected = "0.213 0.458 0.614";
    std::string result = std::format("{:0.3f}", c);    
    CHECK(result == expected);

    // more complex case, with characters around
    std::string result_complex = std::format("RGB: [{}]", c);
    CHECK(result_complex == "RGB: [0.2135 0.45751 0.6144]");
}

// =============================================================================
// Testing Color::luminosity() methods
// =============================================================================

TEST_CASE("Testing Color luminosity methods")
{
    Color c{0.8f, 0.0f, 1.0f};

    SUBCASE("Mid-range luminosity") {
        CHECK(aux::are_close(c.luminosity_mid_range(), 0.5f, 1e-5f));
    }

    SUBCASE("Arithmetic mean luminosity") {
        CHECK(aux::are_close(c.luminosity_arithmetic_mean(), 0.6, 1e-5f));
    }

    SUBCASE("BT.709 luminosity") {
        CHECK(aux::are_close(c.luminosity_bt709(), (c.r * 0.2126f) + (c.g * 0.7152f) + (c.b * 0.0722f), 1e-5f));
    }
}