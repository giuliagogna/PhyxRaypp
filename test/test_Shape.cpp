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
import auxiliary_functions;

// ====================== HITRECORD STRUCT TESTS =================================
// =========================================================================
// TEST 1: Testing if two record are close (is_close())
// =========================================================================

TEST_CASE("TEST 1: Similarity between two HitRecord objects (is_close())") {

    Ray ray1{Point{0.0f, 0.0f, 0.0f}, Vec{1.0f, 2.0f, 3.0f}};
    Ray ray2{Point{0.0f, 0.0f, 0.0f}, Vec{1.0f, 2.0f, 3.0f}};
    Ray ray3{Point{1.0f, 0.0f, 0.0f}, Vec{1.0f, 2.0f, 3.0f}};

    Normal normal1{0.0f, 1.0f, 0.0f};
    Normal normal2{0.0f, 1.0f, 0.0f};
    Normal normal3{1.0f, 0.0f, 0.0f};

    Vec2D uv1{0.5f, 0.5f};
    Vec2D uv2{0.5f, 0.5f};
    Vec2D uv3{0.1f, 0.1f};
    //std::pair<float, float> uv1{0.5f, 0.5f};
    //std::pair<float, float> uv2{0.5f, 0.5f};
    //std::pair<float, float> uv3{0.1f, 0.1f};

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

// ====================== SPHERE STRUCT TESTS ==============================

TEST_CASE("TEST 2: Sphere Test Suite") {
    SUBCASE("Correct translation and scaling in the constructor") {
       Sphere sphere(Point{1.0f, 2.0f, 3.0f}, 4.0f);
       // The transformation should scale by 4 and translate by -origin
       Transformation expected = Scale(Vec{0.25f, 0.25f, 0.25f}) * Trans(Vec{-1.0f, -2.0f, -3.0f});
       CHECK(sphere.trans.m.is_close(expected.m));
   }

   Sphere sphere(Point{0.0f, 0.0f, 0.0f}, 1.0f);

   SUBCASE("Ray-sphere intersection: unitary sphere at origin") {
       Ray ray{Point{0.0f, 0.0f, 2.0f}, Vec{0.0f, 0.0f, -1.0f}};
       auto hit = sphere.ray_intersection(ray);

       REQUIRE(hit.has_value());
       CHECK(hit->hit_point.is_close(Point{0.0f, 0.0f, 1.0f}));
       CHECK(hit->hit_normal.is_close(Normal{0.0f, 0.0f, 1.0f}));
       //CHECK(aux::are_close(hit->uv.first, 0.0f)); 
       CHECK(aux::are_close(hit->surface_params.v, 0.0f));
   }

   SUBCASE("Unitary sphere intersection: no intersection") {
       Ray ray{Point{0.0f, 0.0f, 2.0f}, Vec{1.0f, 0.0f, 0.0f}};
       auto hit = sphere.ray_intersection(ray);
       CHECK(hit.has_value() == false);
   }

   SUBCASE("Unitary sphere intersection: ray originates inside the sphere") {
       Ray ray{Point{0.0f, 0.0f, 0.5f}, Vec{0.0f, 0.0f, -1.0f}};
       auto hit = sphere.ray_intersection(ray);

       REQUIRE(hit.has_value());
       CHECK(hit->hit_point.is_close(Point{0.0f, 0.0f, -1.0f}));
       CHECK(hit->hit_normal.is_close(Normal{0.0f, 0.0f, 1.0f}));
       //CHECK(aux::are_close(hit->uv.first, 1.0f)); 
       CHECK(aux::are_close(hit->surface_params.v, 1.0f));
   }

   Sphere sphere2(Point{1.0f, 2.0f, 3.0f}, 2.0f);

   SUBCASE("Non-unitary sphere intersection") {
        
        std::println("{}", sphere2.trans.m.mat);
        Ray ray{Point{1.0f, -1.0f, 3.0f}, Vec{0.0f, 1.0f, 0.0f}};
        auto hit = sphere2.ray_intersection(ray);

        REQUIRE(hit.has_value());
        std::println("{} {}", hit->hit_point, hit->hit_normal);
        CHECK(hit->hit_point.is_close(Point{1.0f, 0.0f, 3.0f}));
        CHECK(hit->hit_normal.is_close(Normal{0.0f, -1.0f, 0.0f}));
        CHECK(aux::are_close(hit->surface_params.u, 0.25f));
        CHECK(aux::are_close(hit->surface_params.v, 0.5f));
        CHECK(aux::are_close(hit->t, 1.0f));
    }


}

// ========================== PLANE STRUCT TESTS ===========================

TEST_CASE("TEST 3: Plane - Comprehensive Test Suite") {
    // SETUP
    // This canonical plane is initialized once here, and doctest will
    // automatically reset its state before executing each SUBCASE.
    Plane plane;


    // Intersections of ray with plane on z=0
    SUBCASE("Intersection from above") {
        Ray ray{Point{0.0f, 0.0f, 1.0f}, Vec{0.0f, 0.0f, -1.0f}};
        auto hit = plane.ray_intersection(ray);

        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 1.0f));
        CHECK(hit->hit_point.is_close(Point{0.0f, 0.0f, 0.0f}));
        CHECK(hit->hit_normal.is_close(Normal{0.0f, 0.0f, 1.0f}));
        // GG: need to change this if we choose to implement Vec2D
        CHECK(aux::are_close(hit->surface_params.u, 0.0f));
        CHECK(aux::are_close(hit->surface_params.v, 0.0f));
    }

    SUBCASE("Intersection from below (inversion of the normal)") {
        Ray ray{Point{0.0f, 0.0f, -1.0f}, Vec{0.0f, 0.0f, 1.0f}};
        auto hit = plane.ray_intersection(ray);

        REQUIRE(hit.has_value());
        CHECK(hit->hit_point.is_close(Point{0.0f, 0.0f, 0.0f}));
        CHECK(hit->hit_normal.is_close(Normal{0.0f, 0.0f, -1.0f})); // The normal must be flipped!
    }

    SUBCASE("Ray parallel to the plane") {
        Ray ray{Point{0.0f, 0.0f, 1.0f}, Vec{1.0f, 0.0f, 0.0f}};
        auto hit = plane.ray_intersection(ray);
        CHECK_FALSE(hit.has_value());
    }


    // Parametrized coordinates (u, v) (Tile Pattern)
    SUBCASE("Positive intersection coordinates") {
        Ray ray{Point{1.25f, 2.75f, 1.0f}, Vec{0.0f, 0.0f, -1.0f}};
        auto hit = plane.ray_intersection(ray);

        REQUIRE(hit.has_value());
        // 1.25 - floor(1.25) = 1.25 - 1.0 = 0.25
        // 2.75 - floor(2.75) = 2.75 - 2.0 = 0.75
        CHECK(aux::are_close(hit->surface_params.u, 0.25f));
        CHECK(aux::are_close(hit->surface_params.v, 0.75f));
    }

    SUBCASE("Negative intersection coordinates") {
        Ray ray{Point{-1.25f, -2.75f, 1.0f}, Vec{0.0f, 0.0f, -1.0f}};
        auto hit = plane.ray_intersection(ray);

        REQUIRE(hit.has_value());
        // -1.25 - floor(-1.25) = -1.25 - (-2.0) = 0.75
        // -2.75 - floor(-2.75) = -2.75 - (-3.0) = 0.25
        CHECK(aux::are_close(hit->surface_params.u, 0.75f));
        CHECK(aux::are_close(hit->surface_params.v, 0.25f));
    }


    // Transformations (Translation and Rotation)
    SUBCASE("Plane translated along axis z") {
        // Mutating the canonical plane's transformation for this subcase
        plane.trans = Trans(Vec{0.0f, 0.0f, 2.0f}); // Plane at z=2
        Ray ray{Point{0.0f, 0.0f, 3.0f}, Vec{0.0f, 0.0f, -1.0f}};
        auto hit = plane.ray_intersection(ray);

        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 1.0f));
        CHECK(hit->hit_point.is_close(Point{0.0f, 0.0f, 2.0f}));
        CHECK(hit->hit_normal.is_close(Normal{0.0f, 0.0f, 1.0f}));
    }

    SUBCASE("Plane rotated around y axis") {
        // Rotating by 90 degrees (pi/2) around y, the xy plane becomes the yz plane (x=0)
        plane.trans = R_y(std::numbers::pi_v<float> / 2.0f);
        Ray ray{Point{2.0f, 0.0f, 0.0f}, Vec{-1.0f, 0.0f, 0.0f}};
        auto hit = plane.ray_intersection(ray);

        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 2.0f));
        CHECK(hit->hit_point.is_close(Point{0.0f, 0.0f, 0.0f}));

        // The normal undergoes the transformation and now points towards the positive x axis
        CHECK(hit->hit_normal.is_close(Normal{1.0f, 0.0f, 0.0f}));
    }
}


// ======================== WORLD STRUCT TESTS =============================

TEST_CASE("World - Testing Ray Intersection and Scene Management") {
    // SETUP: Create an empty world.
    // doctest will reset this to empty before each SUBCASE.
    World world;

    SUBCASE("Empty World returns no intersection") {
        // World has default initialization, empty list
        Ray ray{Point{0.0f, 0.0f, 0.0f}, Vec{0.0f, 0.0f, 1.0f}};
        auto hit = world.ray_intersection(ray);

        CHECK_FALSE(hit.has_value());
    }

    SUBCASE("World with a single plane") {
        // Add a plane translated to z = +2
        world.add(std::make_unique<Plane>(Trans(Vec{0.0f, 0.0f, 2.0f})));

        Ray ray{Point{1.0f, 2.0f, 0.0f}, Vec{0.0f, 0.0f, 1.0f}};
        auto hit = world.ray_intersection(ray);

        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 2.0f));
        CHECK(hit->hit_point.is_close(Point{1.0f, 2.0f, 2.0f}));
    }

    SUBCASE("World with multiple planes (Closest hit logic)") {
        // Add a plane at z = +5 (Add first)
        world.add(std::make_unique<Plane>(Trans(Vec{0.0f, 0.0f, 5.0f})));

        // Add a plane at z = +2 (Add second)
        world.add(std::make_unique<Plane>(Trans(Vec{0.0f, 0.0f, 2.0f})));

        Ray ray{Point{0.0f, 0.0f, 0.0f}, Vec{0.0f, 0.0f, 1.0f}};
        auto hit = world.ray_intersection(ray);

        // The ray should hit the plane at z=2 first, ignoring the plane at z=5
        // This proves the closest-hit algorithm works correctly.
        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 2.0f));
        CHECK(hit->hit_point.is_close(Point{0.0f, 0.0f, 2.0f}));
    }

    SUBCASE("World with multiple planes but ray misses all") {
        // Add two planes
        world.add(std::make_unique<Plane>(Trans(Vec{0.0f, 0.0f, 5.0f})));
        world.add(std::make_unique<Plane>(Trans(Vec{0.0f, 0.0f, 2.0f})));

        // Shoot a ray parallel to the XY planes (along the X axis, starting at z=1)
        Ray ray{Point{0.0f, 0.0f, 1.0f}, Vec{1.0f, 0.0f, 0.0f}};
        auto hit = world.ray_intersection(ray);

        CHECK_FALSE(hit.has_value());
    }

    SUBCASE("World with multiple spheres (Closest hit logic)") {
        world.add(std::make_unique<Sphere>(Point{0.0f, 0.0f, 5.0f}, 2.0f));
        world.add(std::make_unique<Sphere>(Point{0.0f, 0.0f, 2.0f}, 2.0f));

        Ray ray{Point{0.0f, 0.0f, -1.0f}, Vec{0.0f, 0.0f, 1.0f}};
        auto hit = world.ray_intersection(ray);
        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 1.0f));
        CHECK(hit->hit_point.is_close(Point{0.0f, 0.0f, 0.0f}));
        CHECK(aux::are_close(hit->surface_params.u, 0.5f));
        CHECK(aux::are_close(hit->surface_params.v, 1.0f));
    }

    SUBCASE("World with multiple shapes (Planes and Spheres): ray lies on a plane") {
        world.add(std::make_unique<Plane>(Trans(Vec{0.0f, 0.0f, 5.0f})));
        world.add(std::make_unique<Sphere>(Point{0.0f, 0.0f, 2.0f}, 2.0f));
        world.add(std::make_unique<Sphere>(Point{0.0f, 0.0f, 5.0f}, 1.0f));
        world.add(std::make_unique<Plane>(Trans(Vec{0.0f, 0.0f, 2.0f})));

        Ray ray{Point{0.0f, 0.0f, 2.0f}, Vec{1.0f, 0.0f, 0.0f}};
        auto hit = world.ray_intersection(ray);
        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 2.0f));
        CHECK(hit->hit_point.is_close(Point{2.0f, 0.0f, 2.0f}));
        CHECK(hit->hit_normal.is_close(Normal{-1.0f, 0.0f, 0.0f}));        
    }
}
