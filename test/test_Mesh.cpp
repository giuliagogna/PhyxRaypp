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
import Geometry;
import Camera;
import auxiliary_functions;
import Color;
import HDRImage;
import Material;
import Pigment;
import BRDF;
import Shape;
import Mesh;


TEST_CASE("TEST 1: BVHAABB test suite") {

    static BVHAABB aabb; // Static because I want to preserve changes through the SUBCASEs

    SUBCASE("Test BVHAABB::grow()") {
        aabb.grow(Point{1.0f, 1.0f, 1.0f});
        CHECK(aabb.minPoint.is_close(Point{1.0f, 1.0f, 1.0f}));
        CHECK(aabb.maxPoint.is_close(Point{1.0f, 1.0f, 1.0f}));
        aabb.grow(Point{-1.0f, -1.0f, -1.0f});
        CHECK(aabb.minPoint.is_close(Point{-1.0f, -1.0f, -1.0f}));
        CHECK(aabb.maxPoint.is_close(Point{1.0f, 1.0f, 1.0f}));

        BVHAABB aabb_2;
        aabb_2.grow(aabb);
        CHECK(aabb.minPoint.is_close(Point{-1.0f, -1.0f, -1.0f}));
        CHECK(aabb.maxPoint.is_close(Point{1.0f, 1.0f, 1.0f}));
    }

    SUBCASE("Test BVHAABB::area()") {        
        CHECK(aux::are_close(aabb.area(), 4.0f * 6.0f));
    }

    SUBCASE("Test BVHAABB::longestAxis()") {
        BVHAABB aabb_3 = aabb;
        aabb_3.grow(Point{2.0f, 0.0f, 0.0f});
        CHECK(aabb_3.longestAxis() == 0);
    }

    SUBCASE("Test BVHAABB::intersect()") {
        Ray ray{Point{0.0f, 0.0f, 2.0f}, Vec{0.0f, 0.0f, -1.0f}};
        CHECK(aabb.intersect(ray));
    }
}

static BVHNode first_node{}; // Static because I want to preserve changes through the SUBCASEs
static std::vector<TrianglePoint> triangle_points;
static std::vector<TriangleIndexes> triangle_point_indexes;
static std::vector<BVHNode> all_nodes;

TEST_CASE("TEST 2: BVHNode test suite") {    

    triangle_points.push_back({Point{ 1.0f + 0.1f,  1.0f,  1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{ 1.0f + 0.1f,  1.0f, -1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{ 1.0f + 0.1f, -1.0f,  1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{ 1.0f + 0.1f, -1.0f, -1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f + 0.1f,  1.0f,  1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f + 0.1f,  1.0f, -1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f + 0.1f, -1.0f,  1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f + 0.1f, -1.0f, -1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});

    triangle_points.push_back({Point{ 1.0f,  1.0f + 0.1f,  1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{ 1.0f,  1.0f + 0.1f, -1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{ 1.0f, -1.0f + 0.1f,  1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{ 1.0f, -1.0f + 0.1f, -1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f,  1.0f + 0.1f,  1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f,  1.0f + 0.1f, -1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f, -1.0f + 0.1f,  1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f, -1.0f + 0.1f, -1.0f}, Normal{ 0.0f,  0.0f,  1.0f}});

    triangle_points.push_back({Point{ 1.0f,  1.0f,  1.0f + 0.1f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{ 1.0f,  1.0f, -1.0f + 0.1f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{ 1.0f, -1.0f,  1.0f + 0.1f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{ 1.0f, -1.0f, -1.0f + 0.1f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f,  1.0f,  1.0f + 0.1f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f,  1.0f, -1.0f + 0.1f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f, -1.0f,  1.0f + 0.1f}, Normal{ 0.0f,  0.0f,  1.0f}});
    triangle_points.push_back({Point{-1.0f, -1.0f, -1.0f + 0.1f}, Normal{ 0.0f,  0.0f,  1.0f}});
    
    triangle_point_indexes.push_back(TriangleIndexes{0, 8, 16});
    triangle_point_indexes.push_back(TriangleIndexes{1, 9, 17});
    triangle_point_indexes.push_back(TriangleIndexes{2, 10, 18});
    triangle_point_indexes.push_back(TriangleIndexes{3, 11, 19});
    triangle_point_indexes.push_back(TriangleIndexes{4, 12, 20});
    triangle_point_indexes.push_back(TriangleIndexes{5, 13, 21});
    triangle_point_indexes.push_back(TriangleIndexes{6, 14, 22});
    triangle_point_indexes.push_back(TriangleIndexes{7, 15, 23});
    

    BVHAABB first_bounds;
    for (auto& point : triangle_points) {
        first_bounds.grow(point.point);
    }
    REQUIRE(first_bounds.maxPoint.is_close(Point{1.1f, 1.1f, 1.1f}));   
    REQUIRE(first_bounds.minPoint.is_close(Point{-1.0f, -1.0f, -1.0f}));

    first_node.bounds = first_bounds;
    first_node.minIndex = 0;
    first_node.maxIndex = static_cast<int>(triangle_point_indexes.size());
    all_nodes.push_back(first_node);
    all_nodes[0].Extend_tree_wrapper(all_nodes, triangle_points, triangle_point_indexes, 2, 1);
    
    SUBCASE("Test BVHNode::Extend_tree()") {
        CHECK(all_nodes.size() == 15); // 1 + 2 + 4 + 8 = 15
    }
}

TEST_CASE("TEST 3: Mesh test suite") {
    Mesh mesh {
        Transformation{}, 
        std::make_shared<Material>(), // <--- Sostituisci Material{} con questo
        triangle_points, 
        triangle_point_indexes, 
        all_nodes
    };
    Ray test_ray{Point{1.01f, 1.01f, 2.0f}, Vec{0.0f, 0.0f, -1.0f}};
    auto record = mesh.ray_intersection(test_ray);
    CHECK(record.has_value());
    HitRecord hit_record = record.value();
    CHECK(hit_record.is_close(hit_record.hitted_shape->ray_intersection(test_ray).value()));
    CHECK(hit_record.hit_normal.is_close(Normal{0.0f, 0.0f, 1.0f}));
    CHECK(hit_record.hit_point.is_close(Point{1.01f, 1.01f, 1.08f}));
    CHECK(aux::are_close(hit_record.t, 0.92f));
}


    