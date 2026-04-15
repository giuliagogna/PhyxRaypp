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
import Camera;
import Geometry;

// ====================== RAY STRUCT TESTS =================================
// =========================================================================
// TEST 1: Testing if two rays are close (is_close())
// =========================================================================

TEST_CASE("Similarity between two Ray objects (is_close())") {
   Ray ray1{Point(1.0, 2.0, 3.0), Vec(5.0, 4.0, -1.0)};
   Ray ray2{Point(1.0, 2.0, 3.0), Vec(5.0, 4.0, -1.0)};
   Ray ray3{Point(5.0, 1.0, 4.0), Vec(3.0, 9.0, 4.0)};

    SUBCASE("Teast with default tolerance") {
       CHECK(ray1.is_close(ray2) == true);
       CHECK(ray1.is_close(ray3) == false);
    }

    SUBCASE("Test with custom tolerance") {
        CHECK(ray1.is_close(ray2, 1e-2f) == true);
        CHECK(ray1.is_close(ray3, 1e-2f) == false);
    }

}

// =========================================================================
// TEST 2: Testing position at a certain t (at())
// =========================================================================

TEST_CASE("Point of arrival after t (at())") {
   Ray ray{Point(1.0, 2.0, 4.0), Vec(4.0, 2.0, 1.0)};
   CHECK(ray.at(0.0).is_close(ray.origin) == true);
   CHECK(ray.at(1.0).is_close(Point(5.0, 4.0, 5.0)) == true);
   CHECK(ray.at(2.0).is_close(Point(9.0, 6.0, 6.0)) == true);

}

/ =========================================================================
// TEST 3: Testing transformation of rays (transform())
// =========================================================================

TEST_CASE("Transformation of a Ray (transform())") {
   Ray ray{Point(1.0, 2.0, 3.0), Vec(6.0, 5.0, 4.0)};
   Transformation tr = Tras(Vec(10.0, 11.0, 12.0)) * R_x(M_PI/2.0f);
   Ray transformed = ray.transform(tr);
   CHECK(transformed.origin.is_close(Point(11.0, 8.0, 14.0)));
   CHECK(transformed.direction.is_close(Vec(6.0, -4.0, 5.0)));

}