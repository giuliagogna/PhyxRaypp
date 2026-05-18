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

    Sphere sphere;

    // Constructor for Sphere passing the transformation
    Transformation tr = Trans(Vec{10.0f, 0.0f, 0.0f});
    Sphere translated_sphere(tr);

    SUBCASE("Intersection from above") {
        Ray direct_ray(Point{0.0f, 0.0f, 2.0f}, Vec{0.0f, 0.0f, -1.0f});
        auto intersection = sphere.ray_intersection(direct_ray);

        REQUIRE(intersection.has_value());

        CHECK(intersection->hit_point.is_close(Point{0.0f, 0.0f, 1.0f}));
        CHECK(intersection->hit_normal.is_close(Normal{0.0f, 0.0f, 1.0f}));
        CHECK(aux::are_close(intersection->surface_params.u, 0.0f)); // atan2(0/0) = 0
        CHECK(aux::are_close(intersection->surface_params.v, 0.0f));
        CHECK(aux::are_close(intersection->t, 1.0f));
    }

    SUBCASE("Intersection from the front") {
        Ray direct_ray(Point{3.0f, 0.0f, 0.0f}, Vec{-1.0f, 0.0f, 0.0f});
        auto intersection = sphere.ray_intersection(direct_ray);

        REQUIRE(intersection.has_value());

        CHECK(intersection->hit_point.is_close(Point{1.0f, 0.0f, 0.0f}));
        CHECK(intersection->hit_normal.is_close(Normal{1.0f, 0.0f, 0.0f}));
        CHECK(aux::are_close(intersection->surface_params.u, 0.0f));
        CHECK(aux::are_close(intersection->surface_params.v, 0.5f));
        CHECK(aux::are_close(intersection->t, 2.0f));
    }

    SUBCASE("Intersection from inside") {
        Ray direct_ray(Point{0.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f});
        auto intersection = sphere.ray_intersection(direct_ray);

        REQUIRE(intersection.has_value());

        CHECK(intersection->hit_point.is_close(Point{1.0f, 0.0f, 0.0f}));
        CHECK(intersection->hit_normal.is_close(Normal{-1.0f, 0.0f, 0.0f}));
        CHECK(aux::are_close(intersection->surface_params.u, 0.0f));
        CHECK(aux::are_close(intersection->surface_params.v, 0.5f));
        CHECK(aux::are_close(intersection->t, 1.0f));
    }

    SUBCASE("Intersection on translated sphere Ray(Point{10.0f, 0.0f, 2.0f}, Vec{0.0f, 0.0f, -1.0f})") {
        Ray direct_ray(Point{10.0f, 0.0f, 2.0f}, Vec{0.0f, 0.0f, -1.0f});
        auto intersection = translated_sphere.ray_intersection(direct_ray);

        REQUIRE(intersection.has_value());

        CHECK(intersection->hit_point.is_close(Point{10.0f, 0.0f, 1.0f}));
        CHECK(intersection->hit_normal.is_close(Normal{0.0f, 0.0f, 1.0f}));
        CHECK(aux::are_close(intersection->surface_params.u, 0.0f));
        CHECK(aux::are_close(intersection->surface_params.v, 0.0f));
        CHECK(aux::are_close(intersection->t, 1.0f));
    }

    SUBCASE("Intersection on translated sphere Ray(Point{13.0f, 0.0f, 0.0f}, Vec{-1.0f, 0.0f, 0.0f})") {
        Ray direct_ray(Point{13.0f, 0.0f, 0.0f}, Vec{-1.0f, 0.0f, 0.0f});
        auto intersection = translated_sphere.ray_intersection(direct_ray);

        REQUIRE(intersection.has_value());

        CHECK(intersection->hit_point.is_close(Point{11.0f, 0.0f, 0.0f}));
        CHECK(intersection->hit_normal.is_close(Normal{1.0f, 0.0f, 0.0f}));
        CHECK(aux::are_close(intersection->surface_params.u, 0.0f));
        CHECK(aux::are_close(intersection->surface_params.v, 0.5f));
        CHECK(aux::are_close(intersection->t, 2.0f));
    }

    SUBCASE("CHeck no intersection with sphere") {
        Ray direct_ray1(Point{0.0f, 0.0f, 2.0f}, Vec{0.0f, 0.0f, -1.0f});
        auto intersection1 = translated_sphere.ray_intersection(direct_ray1);

        Ray direct_ray2(Point{-10.0f, 0.0f, 2.0f}, Vec{0.0f, 0.0f, -1.0f});
        auto intersection2 = translated_sphere.ray_intersection(direct_ray2);

        CHECK_FALSE(intersection1.has_value());
        CHECK_FALSE(intersection2.has_value());
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

// ======================== CUBE STRUCT TESTS =============================
TEST_CASE("TEST 4: CUBE - Comprehensive Test Suite") {
    Cube cube;

    SUBCASE("Rays perpendicular to the faces") {
        Ray ray1{Point{2.0f, 0.0f, 0.0f}, Vec{-1.0f, 0.0f, 0.0f}};
        Ray ray2{Point{-2.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};
        Ray ray3{Point{0.0f, 2.0f, 0.0f}, Vec{0.0f, -1.0f, 0.0f}};
        Ray ray4{Point{0.0f, -2.0f, 0.0f}, Vec{0.0f, 1.0f, 0.0f}};
        Ray ray5{Point{0.0f, 0.0f, 2.0f}, Vec{0.0f, 0.0f, -1.0f}};
        Ray ray6{Point{0.0f, 0.0f, -2.0f}, Vec{0.0f, 0.0f, 1.0f}};

        auto record1 = cube.ray_intersection(ray1);
        auto record2 = cube.ray_intersection(ray2);
        auto record3 = cube.ray_intersection(ray3);
        auto record4 = cube.ray_intersection(ray4);
        auto record5 = cube.ray_intersection(ray5);
        auto record6 = cube.ray_intersection(ray6);

        REQUIRE(record1.has_value());
        REQUIRE(record2.has_value());
        REQUIRE(record3.has_value());
        REQUIRE(record4.has_value());
        REQUIRE(record5.has_value());
        REQUIRE(record6.has_value());

        HitRecord exp1{
            ray1,
            Point{1.0f, 0.0f, 0.0f},
            Normal{1.0f, 0.0f, 0.0f},
            Vec2D{0.5f, 0.5f},
            1.0f
        };
        HitRecord exp2{
            ray2,
            Point{-1.0f, 0.0f, 0.0f},
            Normal{-1.0f, 0.0f, 0.0f},
            Vec2D{0.5f, 0.5f},
            1.0f
        };
        HitRecord exp3{
            ray3,
            Point{0.0f, 1.0f, 0.0f},
            Normal{0.0f, 1.0f, 0.0f},
            Vec2D{0.5f, 0.5f},
            1.0f
        };
        HitRecord exp4{
            ray4,
            Point{0.0f, -1.0f, 0.0f},
            Normal{0.0f, -1.0f, 0.0f},
            Vec2D{0.5f, 0.5f},
            1.0f
        };
        HitRecord exp5{
            ray5,
            Point{0.0f, 0.0f, 1.0f},
            Normal{0.0f, 0.0f, 1.0f},
            Vec2D{0.5f, 0.5f},
            1.0f
        };
        HitRecord exp6{
            ray6,
            Point{0.0f, 0.0f, -1.0f},
            Normal{0.0f, 0.0f, -1.0f},
            Vec2D{0.5f, 0.5f},
            1.0f
        };

        CHECK(record1->is_close(exp1));
        CHECK(record2->is_close(exp2));
        CHECK(record3->is_close(exp3));
        CHECK(record4->is_close(exp4));
        CHECK(record5->is_close(exp5));
        CHECK(record6->is_close(exp6));

        // Test each component of the HitRecord object individually to better debug
        // Ray
        CHECK(record1->ray.is_close(exp1.ray));
        CHECK(record2->ray.is_close(exp2.ray));
        CHECK(record3->ray.is_close(exp3.ray));
        CHECK(record4->ray.is_close(exp4.ray));
        CHECK(record5->ray.is_close(exp5.ray));
        CHECK(record6->ray.is_close(exp6.ray));

        // Hit Point
        CHECK(record1->hit_point.is_close(exp1.hit_point));
        CHECK(record2->hit_point.is_close(exp2.hit_point));
        CHECK(record3->hit_point.is_close(exp3.hit_point));
        CHECK(record4->hit_point.is_close(exp4.hit_point));
        CHECK(record5->hit_point.is_close(exp5.hit_point));
        CHECK(record6->hit_point.is_close(exp6.hit_point));

        // Hit Normal
        CHECK(record1->hit_normal.is_close(exp1.hit_normal));
        CHECK(record2->hit_normal.is_close(exp2.hit_normal));
        CHECK(record3->hit_normal.is_close(exp3.hit_normal));
        CHECK(record4->hit_normal.is_close(exp4.hit_normal));
        CHECK(record5->hit_normal.is_close(exp5.hit_normal));
        CHECK(record6->hit_normal.is_close(exp6.hit_normal));

        // Surface params
        CHECK(record1->surface_params.is_close(exp1.surface_params));
        CHECK(record2->surface_params.is_close(exp2.surface_params));
        CHECK(record3->surface_params.is_close(exp3.surface_params));
        CHECK(record4->surface_params.is_close(exp4.surface_params));
        CHECK(record5->surface_params.is_close(exp5.surface_params));
        CHECK(record6->surface_params.is_close(exp6.surface_params));

        // t
        CHECK(aux::are_close(record1->t, exp1.t));
        CHECK(aux::are_close(record2->t, exp2.t));
        CHECK(aux::are_close(record3->t, exp3.t));
        CHECK(aux::are_close(record4->t, exp4.t));
        CHECK(aux::are_close(record5->t, exp5.t));
        CHECK(aux::are_close(record6->t, exp6.t));

    }

    SUBCASE("Missed intersection - Ray parallel to a face outside the cube") {
        Ray ray_miss{Point{2.0f, 5.0f, 3.0}, Vec{-1.0f, 0.0f, 0.0f}};
        auto record = cube.ray_intersection(ray_miss);

        CHECK_FALSE(record.has_value());
    }

    SUBCASE("Missed interection - Ray with direction outward outside the cube") {
        Ray ray_miss{Point{2.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};
        auto record = cube.ray_intersection(ray_miss);
        CHECK_FALSE(record.has_value());
    }

    SUBCASE("Ray from inside the cube") {
        Ray ray{Point{0.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};
        auto record = cube.ray_intersection(ray);

        REQUIRE(record.has_value());
        CHECK(record->ray.is_close(ray));
        CHECK(record->hit_point.is_close(Point(1.0f, 0.0f, 0.0f)));
        CHECK(record->hit_normal.is_close(Normal{-1.0f, 0.0f, 0.0f}));
        CHECK(record->surface_params.is_close(Vec2D{0.5f, 0.5f}));
        CHECK(aux::are_close(record->t, 1.0f));
    }

    SUBCASE("UV parametrization - X face") {
        Ray ray{Point{3.0f, 0.5f, -0.5f}, Vec{-1.0f, 0.0f, 0.0f}};
        auto record = cube.ray_intersection(ray);

        REQUIRE(record.has_value());
        CHECK(record->surface_params.is_close(Vec2D{0.75f, 0.25f}));
    }

    SUBCASE("UV parametrization - Z face") {
        Ray ray{Point{0.5f, 0.5f, 3.0f}, Vec{0.0f, 0.0f, -1.0f}};
        auto record = cube.ray_intersection(ray);

        REQUIRE(record.has_value());
        CHECK(record->surface_params.is_close(Vec2D{0.75f, 0.25f}));
    }

    SUBCASE("Cube transformation") {
        Cube cube_transformed{Trans(Vec{5.0f, 0.0f, 0.0f})*Scale(Vec{2.0f, 2.0f, 2.0f})};
        Ray ray{Point{0.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};

        auto record = cube_transformed.ray_intersection(ray);
        HitRecord exp{
            ray,
            Point{3.0f, 0.0f, 0.0f},
            Normal{-1.0f, 0.0f, 0.0f},
            Vec2D{0.5f, 0.5f},
            3.0f
        };

        REQUIRE(record.has_value());
        CHECK(record->is_close(exp));
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
        world.add(std::make_unique<Sphere>(Trans(Vec{0.0f, 0.0f, 5.0f}) * Scale(Vec{2.0f, 2.0f, 2.0f})));
        world.add(std::make_unique<Sphere>(Trans(Vec{0.0f, 0.0f, 2.0f}) * Scale(Vec{2.0f, 2.0f, 2.0f})));

        Ray ray{Point{0.0f, 0.0f, -1.0f}, Vec{0.0f, 0.0f, 1.0f}};
        auto hit = world.ray_intersection(ray);
        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 1.0f));
        CHECK(hit->hit_point.is_close(Point{0.0f, 0.0f, 0.0f}));
        CHECK(aux::are_close(hit->surface_params.u, 0.0f));
        CHECK(aux::are_close(hit->surface_params.v, 1.0f));
    }

    SUBCASE("World with multiple shapes (Planes and Spheres): ray lies on a plane") {

        world.add(std::make_unique<Plane>(Trans(Vec{0.0f, 0.0f, 5.0f})));
        world.add(std::make_unique<Plane>(Trans(Vec{0.0f, 0.0f, 2.0f})));
        // Translated and scaled sphere
        world.add(std::make_unique<Sphere>(Trans(Vec{0.0f, 0.0f, 2.0f}) * Scale(Vec{2.0f, 2.0f, 2.0f})));

        Ray ray{Point{0.0f, 0.0f, 2.0f}, Vec{1.0f, 0.0f, 0.0f}};
        auto hit = world.ray_intersection(ray);
        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 2.0f));
        CHECK(hit->hit_point.is_close(Point{2.0f, 0.0f, 2.0f}));
        CHECK(hit->hit_normal.is_close(Normal{-1.0f, 0.0f, 0.0f}));
    }
}
