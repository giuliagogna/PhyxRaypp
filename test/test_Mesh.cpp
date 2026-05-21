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

TEST_CASE("TEST 2: BVHNode test suite") {
    Transformation rotation = R_x(0.1f);
    std::vector<TrianglePoint> triangle_points;
    triangle_points.push_back({rotation * Point{ 0.0f,  0.0f,  1.0f}, rotation * Normal{ 0.0f,  0.0f,  1.0f}.normalize()});
    triangle_points.push_back({rotation * Point{-1.0f, -1.0f,  0.0f}, rotation * Normal{-1.0f, -1.0f,  0.0f}.normalize()});
    triangle_points.push_back({rotation * Point{-1.0f,  1.0f,  0.0f}, rotation * Normal{-1.0f,  1.0f,  0.0f}.normalize()});
    triangle_points.push_back({rotation * Point{ 1.0f,  1.0f,  0.0f}, rotation * Normal{ 1.0f, -1.0f,  0.0f}.normalize()});
    triangle_points.push_back({rotation * Point{ 1.0f,  -1.0f,  0.0f}, rotation * Normal{ 1.0f,  1.0f,  0.0f}.normalize()});
    triangle_points.push_back({rotation * Point{ 0.0f,  0.0f, -1.0f}, rotation * Normal{ 0.0f,  0.0f, -1.0f}.normalize()});
    
    std::vector<TriangleIndexes> triangle_point_indexes;
    triangle_point_indexes.push_back(TriangleIndexes{0, 1, 2});
    triangle_point_indexes.push_back(TriangleIndexes{0, 2, 3});
    triangle_point_indexes.push_back(TriangleIndexes{0, 3, 4});
    triangle_point_indexes.push_back(TriangleIndexes{0, 4, 1});
    triangle_point_indexes.push_back(TriangleIndexes{5, 1, 2});
    triangle_point_indexes.push_back(TriangleIndexes{5, 2, 3});
    triangle_point_indexes.push_back(TriangleIndexes{5, 3, 4});
    triangle_point_indexes.push_back(TriangleIndexes{5, 4, 1});

    BVHAABB first_bounds;
    for (auto& point : triangle_points) {
        first_bounds.grow(point.point);
    }
    REQUIRE(first_bounds.maxPoint.is_close(rotation * Point{1.0f, 1.0f, 1.0f}, 1e-1));    // Not easy because I rotated the shape... I just want to check if it's not completely broken
    REQUIRE(first_bounds.minPoint.is_close(rotation * Point{-1.0f, -1.0f, -1.0f}, 1e-1)); // Not easy because I rotated the shape... I just want to check if it's not completely broken

    BVHNode first_node{first_bounds};
    first_node.minIndex = 0;
    first_node.maxIndex = static_cast<int>(triangle_point_indexes.size());
    std::vector<BVHNode> all_nodes;
    all_nodes.push_back(first_node);
    all_nodes[0].Extend_tree_wrapper(all_nodes, triangle_points, triangle_point_indexes, 3, 1);
    
    CHECK(all_nodes.size() == 15);

    for(auto& node : all_nodes) {
        if (node.is_leaf) {
            std::println("== LEAF ==");
        }
        std::println("Node index: {}", &node - &all_nodes[0]);
        std::println("Node bounds: min {}, max {}", node.bounds.minPoint, node.bounds.maxPoint);
        std::println("Node triangle indexes and values:");
        for (int i = node.minIndex; i < node.maxIndex; ++i) {
            const auto& tri = triangle_point_indexes[i];
            std::println("  Triangle indexes: {}, {}, {}", tri.i1, tri.i2, tri.i3);
            std::println("  Triangle points: {}, {}, {}", triangle_points[tri.i1].point, triangle_points[tri.i2].point, triangle_points[tri.i3].point);
        }
    }

}