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
import HDRImage;
import Geometry;
import Camera;
import Shape;

// ====================== RAY STRUCT TESTS =================================
// =========================================================================
// TEST 1: Testing if two rays are close (is_close())
// =========================================================================

TEST_CASE("Similarity between two HitRecord objects (is_close())") {

   Ray ray1{Point{0.0f, 0.0f, 0.0f}, Vec{1.0f, 2.0f, 3.0f}};
   Ray ray2{Point{0.0f, 0.0f, 0.0f}, Vec{1.0f, 2.0f, 3.0f}};
   Ray ray3{Point{1.0f, 0.0f, 0.0f}, Vec{1.0f, 2.0f, 3.0f}};

   Normal normal1{0.0f, 1.0f, 0.0f};
   Normal normal2{0.0f, 1.0f, 0.0f};
   Normal normal3{1.0f, 0.0f, 0.0f};

   std::pair<float, float> uv1{0.5f, 0.5f};
   std::pair<float, float> uv2{0.5f, 0.5f};
   std::pair<float, float> uv3{0.1f, 0.1f};

   HitRecord hit1{ray1, Point{1.0f, 2.0f, 3.0f}, normal1, uv1, 5.0f};
   HitRecord hit2{ray2, Point{1.0f, 2.0f, 3.0f}, normal2, uv2, 5.0f};
   HitRecord hit3{ray3, Point{1.0f, 2.0f, 3.0f}, normal3, uv3, 5.0f};

   SUBCASE("Test with default tolerance") {
      CHECK(hit1.is_close(hit2) == true);
      CHECK(hit1.is_close(hit3) == false);
   }
   SUBCASE("Test with custom tolerance") {
       CHECK(hit1.is_close(hit2, 1e-2f) == true);
       CHECK(hit1.is_close(hit3, 1e-2f) == false);
   }
}
