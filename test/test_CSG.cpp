/*
 * Copyright (c) 2026 Giulia Gogna, Riccardo Piazza.
 * Licensed under the EUPL, Version 1.2
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import std;
import Geometry;
import Camera;
import Shape;
import CSG; 
import auxiliary_functions;

// =========================================================================
// TEST 1: CSG UNION (A ∪ B)
// =========================================================================
TEST_CASE("TEST 1: CSG - Union Operations") {
    auto sphere_A = std::make_unique<Sphere>();
    auto sphere_B = std::make_unique<Sphere>(Trans(Vec{1.0f, 0.0f, 0.0f}));
    CSG csg_union(std::move(sphere_A), std::move(sphere_B), CSGOperations::Union);

    SUBCASE("Ray entering from the left side of Union") {
        Ray ray{Point{-2.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};
        
        auto hit = csg_union.ray_intersection(ray);
        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 1.0f));
        CHECK(hit->hit_point.is_close(Point{-1.0f, 0.0f, 0.0f}));
        CHECK(hit->hit_normal.is_close(Normal{-1.0f, 0.0f, 0.0f}));

        auto all_hits = csg_union.ray_all_intersections(ray);
        REQUIRE(all_hits.size() == 2);

        CHECK(aux::are_close(all_hits[0].t, 1.0f));
        CHECK(all_hits[0].is_entering == true);
        CHECK(all_hits[0].hit_normal.is_close(Normal{-1.0f, 0.0f, 0.0f}));

        CHECK(aux::are_close(all_hits[1].t, 4.0f)); 
        CHECK(all_hits[1].is_entering == false);
        CHECK(all_hits[1].hit_normal.is_close(Normal{-1.0f, 0.0f, 0.0f}));
    }

    SUBCASE("Ray hitting only one of the shapes in the union") {
        Ray ray{Point{-0.01f, 2.0f, 0.0f}, Vec{0.0f, -1.0f, 0.0f}};
        
        auto all_hits = csg_union.ray_all_intersections(ray);
        REQUIRE(all_hits.size() == 2);
        CHECK(aux::are_close(all_hits[0].t, 1.0f, 0.01f));
        CHECK(all_hits[1].t > all_hits[0].t);       
    }

    SUBCASE("Ray missing the union entirely") {
        Ray ray{Point{-2.0f, 5.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};
        auto hit = csg_union.ray_intersection(ray);
        auto all_hits = csg_union.ray_all_intersections(ray);

        CHECK_FALSE(hit.has_value());
        CHECK(all_hits.empty());
    }
}

// =========================================================================
// TEST 2: CSG INTERSECTION (A ∩ B)
// =========================================================================
TEST_CASE("TEST 2: CSG - Intersection Operations") {
    auto sphere_A = std::make_unique<Sphere>();
    auto sphere_B = std::make_unique<Sphere>(Trans(Vec{1.0f, 0.0f, 0.0f}));
    CSG csg_intersection(std::move(sphere_A), std::move(sphere_B), CSGOperations::Intersection);

    SUBCASE("Ray piercing the shared volume") {
        Ray ray{Point{-2.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};

        auto hit = csg_intersection.ray_intersection(ray);
        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 2.0f));
        CHECK(hit->hit_point.is_close(Point{0.0f, 0.0f, 0.0f}));
        CHECK(hit->hit_normal.is_close(Normal{-1.0f, 0.0f, 0.0f}));

        auto all_hits = csg_intersection.ray_all_intersections(ray);
        REQUIRE(all_hits.size() == 2);

        CHECK(aux::are_close(all_hits[0].t, 2.0f));
        CHECK(all_hits[0].is_entering == true);

        CHECK(aux::are_close(all_hits[1].t, 3.0f));
        CHECK(all_hits[1].is_entering == false);
        CHECK(all_hits[1].hit_normal.is_close(Normal{-1.0f, 0.0f, 0.0f}));
    }

    SUBCASE("Ray hitting shapes but skipping overlap region") {
        Ray ray{Point{0.0f, 0.95f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};
        auto hit = csg_intersection.ray_intersection(ray);
        CHECK_FALSE(hit.has_value());
    }
}

// =========================================================================
// TEST 3: CSG DIFFERENCE (A - B)
// =========================================================================
TEST_CASE("TEST 3: CSG - Difference Operations (Carving)") {
    auto sphere_A = std::make_unique<Sphere>();
    auto sphere_B = std::make_unique<Sphere>(Trans(Vec{1.0f, 0.0f, 0.0f}));
    CSG csg_difference(std::move(sphere_A), std::move(sphere_B), CSGOperations::Difference);

    SUBCASE("Ray hitting the carved solid from the left") {
        Ray ray{Point{-2.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};

        auto hit = csg_difference.ray_intersection(ray);
        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 1.0f));
        CHECK(hit->hit_point.is_close(Point{-1.0f, 0.0f, 0.0f}));

        auto all_hits = csg_difference.ray_all_intersections(ray);
        REQUIRE(all_hits.size() == 2);

        CHECK(aux::are_close(all_hits[0].t, 1.0f));
        CHECK(all_hits[0].is_entering == true);

        CHECK(aux::are_close(all_hits[1].t, 2.0f));
        CHECK(all_hits[1].is_entering == false);
        CHECK(all_hits[1].hit_normal.is_close(Normal{1.0f, 0.0f, 0.0f})); // Corrected normal direction for the exit point
    }
}

// =========================================================================
// TEST 4: CSG TRANSFORMATION HIERARCHY
// =========================================================================
TEST_CASE("TEST 4: CSG - Transformation Hierarchy") {
    auto sphere_A = std::make_unique<Sphere>();
    auto sphere_B = std::make_unique<Sphere>(Trans(Vec{1.0f, 0.0f, 0.0f}));
    
    Transformation global_move = Trans(Vec{0.0f, 0.0f, 10.0f});
    CSG csg_transformed(std::move(sphere_A), std::move(sphere_B), CSGOperations::Union);
    csg_transformed.trans = global_move; // Uses trans field inherited from Shape

    SUBCASE("Ray hitting the transformed CSG complex structure") {
        Ray ray{Point{-2.0f, 0.0f, 10.0f}, Vec{1.0f, 0.0f, 0.0f}};

        auto hit = csg_transformed.ray_intersection(ray);
        REQUIRE(hit.has_value());
        
        CHECK(aux::are_close(hit->t, 1.0f));
        CHECK(hit->hit_point.is_close(Point{-1.0f, 0.0f, 10.0f}));
        CHECK(hit->hit_normal.is_close(Normal{-1.0f, 0.0f, 0.0f}));

        auto all_hits = csg_transformed.ray_all_intersections(ray);
        REQUIRE(all_hits.size() == 2);
        CHECK(all_hits[0].hit_point.is_close(Point{-1.0f, 0.0f, 10.0f}));
        CHECK(all_hits[1].hit_point.is_close(Point{2.0f, 0.0f, 10.0f}));
    }
}

// =========================================================================
// TEST 5: NESTED CSG TREE ((A ∪ B) - C)
// =========================================================================
TEST_CASE("TEST 5: CSG - Complex Nested Tree Structure") {
    auto sphere_A = std::make_unique<Sphere>();
    auto sphere_B = std::make_unique<Sphere>(Trans(Vec{1.0f, 0.0f, 0.0f}));
    auto csg_union = std::make_unique<CSG>(std::move(sphere_A), std::move(sphere_B), CSGOperations::Union);
    
    auto cube_C = std::make_unique<Cube>(Trans(Vec{2.0f, 0.0f, 0.0f}));
    CSG csg_root(std::move(csg_union), std::move(cube_C), CSGOperations::Difference);

    SUBCASE("Ray traversing from left to right through the whole complex") {
        Ray ray{Point{-2.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};

        auto hit = csg_root.ray_intersection(ray);
        REQUIRE(hit.has_value());
        CHECK(aux::are_close(hit->t, 1.0f));

        auto all_hits = csg_root.ray_all_intersections(ray);
        REQUIRE(all_hits.size() == 2);

        CHECK(aux::are_close(all_hits[0].t, 1.0f));
        CHECK(all_hits[0].is_entering == true);

        CHECK(aux::are_close(all_hits[1].t, 3.0f)); 
        CHECK(all_hits[1].is_entering == false);
        CHECK(all_hits[1].hit_normal.is_close(Normal{1.0f, 0.0f, 0.0f}));
    }
}

// =========================================================================
// TEST 6: CONCENTRIC SHAPES (Hollow Shell / Glass Bubble)
// =========================================================================
TEST_CASE("TEST 6: CSG - Concentric Shapes (Difference)") {
    auto big_sphere = std::make_unique<Sphere>(Scale(Vec{2.0f, 2.0f, 2.0f}));
    auto small_sphere = std::make_unique<Sphere>(); 
    CSG hollow_shell(std::move(big_sphere), std::move(small_sphere), CSGOperations::Difference);

    SUBCASE("Ray piercing completely through the hollow bubble") {
        Ray ray{Point{-4.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};

        auto all_hits = hollow_shell.ray_all_intersections(ray);
        REQUIRE(all_hits.size() == 4);

        CHECK(aux::are_close(all_hits[0].t, 2.0f));
        CHECK(all_hits[0].is_entering == true);

        CHECK(aux::are_close(all_hits[1].t, 3.0f));
        CHECK(all_hits[1].is_entering == false);
        CHECK(all_hits[1].hit_normal.is_close(Normal{1.0f, 0.0f, 0.0f})); 

        CHECK(aux::are_close(all_hits[2].t, 5.0f));
        CHECK(all_hits[2].is_entering == true);
        CHECK(all_hits[2].hit_normal.is_close(Normal{1.0f, 0.0f, 0.0f})); 

        CHECK(aux::are_close(all_hits[3].t, 6.0f));
        CHECK(all_hits[3].is_entering == false);
    }
}

// =========================================================================
// TEST 7: TOTAL SUBTRACTION (Empty Set Case)
// =========================================================================
TEST_CASE("TEST 7: CSG - Total Subtraction Edge Case") {
    auto sphere_A = std::make_unique<Sphere>();
    auto huge_cube = std::make_unique<Cube>(Scale(Vec{5.0f, 5.0f, 5.0f}));
    CSG empty_solid(std::move(sphere_A), std::move(huge_cube), CSGOperations::Difference);

    SUBCASE("Ray should pass through without hitting anything") {
        Ray ray{Point{-3.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}};

        auto hit = empty_solid.ray_intersection(ray);
        auto all_hits = empty_solid.ray_all_intersections(ray);

        CHECK_FALSE(hit.has_value());
        CHECK(all_hits.empty());
    }
}