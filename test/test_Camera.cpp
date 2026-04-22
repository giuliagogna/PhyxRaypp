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

// ====================== RAY STRUCT TESTS =================================
// =========================================================================
// TEST 1: Testing if two rays are close (is_close())
// =========================================================================

TEST_CASE("Similarity between two Ray objects (is_close())") {
   Ray ray1{Point{1.0f, 2.0f, 3.0f}, Vec{5.0f, 4.0f, -1.0f}};
   Ray ray2{Point{1.0f, 2.0f, 3.0f}, Vec{5.0f, 4.0f, -1.0f}};
   Ray ray3{Point{5.0f, 1.0f, 4.0f}, Vec{3.0f, 9.0f, 4.0f}};

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
   Ray ray{Point{1.0f, 2.0f, 4.0f}, Vec{4.0f, 2.0f, 1.0f}};
   CHECK(ray.at(0.0f).is_close(ray.origin) == true);
   CHECK(ray.at(1.0f).is_close(Point{5.0f, 4.0f, 5.0f}) == true);
   CHECK(ray.at(2.0f).is_close(Point(9.0f, 6.0f, 6.0f)) == true);
}

// =========================================================================
// TEST 3: Testing transformation of rays (transform())
// ========================================================================

TEST_CASE("Transformation of a Ray (transform())") {
   Ray ray{Point(1.0f, 2.0f, 3.0f), Vec(6.0f, 5.0f, 4.0f)};
   Transformation tr = Tras(Vec(10.0f, 11.0f, 12.0f)) * R_x(std::numbers::pi_v<float> / 2.0f);
   Ray transformed = ray.transform(tr);
   CHECK(transformed.origin.is_close(Point(11.0f, 8.0f, 14.0f)));
   CHECK(transformed.direction.is_close(Vec(6.0f, -4.0f, 5.0f)));
}


// ====================== CAMERA STRUCT TESTS ==============================
// =========================================================================
// TEST 4: Orthogonal camera tests
// =========================================================================

TEST_CASE("OrthogonalCamera") {

   SUBCASE("Geometric properties") {
      OrthogonalCamera cam{2.0f}; // Default transformation is Identity
      constexpr float tolerance = 1e-5f;

      Ray ray1 = cam.fire_ray(0.0f, 0.0f);
      Ray ray2 = cam.fire_ray(1.0f, 0.0f);
      Ray ray3 = cam.fire_ray(0.0f, 1.0f);
      Ray ray4 = cam.fire_ray(1.0f, 1.0f);

      // Verify that the rays are parallel by checking that cross-products vanish
      CHECK((ray1.direction % ray2.direction).norm2() < tolerance);
      CHECK((ray1.direction % ray3.direction).norm2() < tolerance);
      CHECK((ray1.direction % ray4.direction).norm2() < tolerance);

      // Verify that the rays hitting the corners have the right coordinates
      CHECK(ray1.at(1.0f).is_close(Point{0.0f, 2.0f, -1.0f}));
      CHECK(ray2.at(1.0f).is_close(Point{0.0f, -2.0f, -1.0f}));
      CHECK(ray3.at(1.0f).is_close(Point{0.0f, 2.0f, 1.0f}));
      CHECK(ray4.at(1.0f).is_close(Point{0.0f, -2.0f, 1.0f}));
   }
   SUBCASE("Transform properties") {
      OrthogonalCamera cam(2.0f, Tras(Vec(10.0f, 20.0f, 30.0f)));
      CHECK(cam.fire_ray(0.0f, 0.0f).is_close(Ray{Point(9, 22, 29), Vec(1, 0, 0)}));
      CHECK(cam.fire_ray(1.0f, 0.0f).is_close(Ray{Point(9, 18, 29), Vec(1, 0, 0)}));
      CHECK(cam.fire_ray(0.0f, 1.0f).is_close(Ray{Point(9, 22, 31), Vec(1, 0, 0)}));
      CHECK(cam.fire_ray(1.0f, 1.0f).is_close(Ray{Point(9, 18, 31), Vec(1, 0, 0)}));
   }

}


// =========================================================================
// TEST 5: Perspective camera tests
// =========================================================================

TEST_CASE("PerspectiveCamera") {

   SUBCASE("Geometric properties") {PerspectiveCamera cam{2.0f, 1.0f}; // Default transformation is Identity
      Ray ray1 = cam.fire_ray(0.0f, 0.0f);
      Ray ray2 = cam.fire_ray(1.0f, 0.0f);
      Ray ray3 = cam.fire_ray(0.0f, 1.0f);
      Ray ray4 = cam.fire_ray(1.0f, 1.0f);

      // Verify that all rays depart from the exact same point
      CHECK(ray1.origin.is_close(ray2.origin));
      CHECK(ray1.origin.is_close(ray3.origin));
      CHECK(ray1.origin.is_close(ray4.origin));

      // Verify that the rays hitting the corners have the right coordinates
      CHECK(ray1.at(1.0f).is_close(Point{0.0f, 2.0f, -1.0f}));
      CHECK(ray2.at(1.0f).is_close(Point{0.0f, -2.0f, -1.0f}));
      CHECK(ray3.at(1.0f).is_close(Point{0.0f, 2.0f, 1.0f}));
      CHECK(ray4.at(1.0f).is_close(Point{0.0f, -2.0f, 1.0f}));
   }

   SUBCASE("Transform properties") {
      PerspectiveCamera cam(1.0f, 0.5f, Tras(Vec(1.0f, 1.0f, 1.0f)));
      CHECK(cam.fire_ray(0.0f, 0.0f).is_close(Ray{Point(0.5f, 1.0f, 1.0f), Vec(0.5f, 1.0f, -1.0f)}));
      CHECK(cam.fire_ray(1.0f, 0.0f).is_close(Ray{Point(0.5f, 1.0f, 1.0f), Vec(0.5f, -1.0f, -1.0f)}));
      CHECK(cam.fire_ray(0.0f, 1.0f).is_close(Ray{Point(0.5f, 1.0f, 1.0f), Vec(0.5f, 1.0f, 1.0f)}));
      CHECK(cam.fire_ray(1.0f, 1.0f).is_close(Ray{Point(0.5f, 1.0f, 1.0f), Vec(0.5f, -1.0f, 1.0f)}));
   }

}


// ===================== IMAGE TRACER TESTS ===============================
// =========================================================================
// TEST 5: Testing ray generation from ImageTracer
// =========================================================================
// GG: CAREFUL! There is an intentional bug in the code as Tomasi pointed out in lesson 6. Need to implement the
//     tests like he did to make sure they pass.
TEST_CASE("Ray generation from ImageTracer (fire_ray())") {

   HDRImage image = HDRImage(4, 2);
   PerspectiveCamera camera = PerspectiveCamera(2.0f);
   ImageTracer tracer = ImageTracer(image, camera);

   SUBCASE("Ray generation from ImageTracer (fire_ray())") {
      Ray ray1 = tracer.fire_ray(0, 0, 2.5, 1.5);
      Ray ray2 = tracer.fire_ray(1, 2, 0.5, 0.5);

      // The two rays hit the same spot on the screen, since u_pixel and v_pixel in ray1 are bigger than 1 starting from (0,0)
      CHECK(ray1.is_close(ray2));
   }
   SUBCASE("Complete mapping (fire_all_rays())") {
      // Call fire_all_rays passing a lambda function that returns a simple color
      tracer.fire_all_rays([](const Ray&) {
        return Color{1.0f, 2.0f, 3.0f};
      });

      const int width = tracer.frame.width;
      const int height = tracer.frame.height;
      // Check that every pixel of the image has been correctly colored
      for (int col = 0; col < width; col++) {
         for (int row = 0; row < height; row++) {
            CHECK(tracer.frame.get_pixel(col, row).is_close(Color{1.0f, 2.0f, 3.0f}));
         }
      }
   }
   SUBCASE("Test frame orientation") {
      Ray top_left_ray = tracer.fire_ray(0, 0, 0.0f, 0.0f);
      Ray bottom_right_ray = tracer.fire_ray(1, 3, 1.0f, 1.0f);
      CHECK(Point(0.0f, 2.0, 1.0f).is_close(top_left_ray.at(1.0f)));
      CHECK(Point(0.0f, -2.0, -1.0f).is_close(bottom_right_ray.at(1.0f)));

   }
}
