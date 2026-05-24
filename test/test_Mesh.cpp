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
    SUBCASE("Test Mesh::ray_intersection") {
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

    SUBCASE("Test Mesh::read_mesh_from_obj(std::istream)") {
        Mesh mesh;

        SUBCASE("Test invalid stream (empty)") {
            std::istringstream empty_stream;
            auto stream_result = mesh.read_mesh_from_obj(empty_stream);
            CHECK(!stream_result.has_value());
            CHECK(stream_result.error() == "Problems with the .obj file");
        }

        SUBCASE("Test error: Points and Normals size mismatch") {
            std::string mismatch_string {R"(
v 0 0 0
v 1 0 0
v 0 1 0
vn 0 0 1
vn 0 0 1
f 1/1/1 2/2/2 3/3/3)"};
            std::istringstream ss(mismatch_string);
            auto result = mesh.read_mesh_from_obj(ss);
            
            CHECK(!result.has_value());
            CHECK(result.error() == "Points and Normals std::vector are different in size, please check the .obj file");
        }

        SUBCASE("Test error: Single triangle vertex and normal index mismatch") {
            std::string index_mismatch_string {R"(
v 0 0 0
v 1 0 0
vn 0 0 1
vn 0 0 1
f 1/1/2 2/2/2 1/1/1)"};
            std::istringstream ss(index_mismatch_string);
            auto result = mesh.read_mesh_from_obj(ss);
            
            CHECK(!result.has_value());
            CHECK(result.error() == "Problems with triangle indexing in .obj file: for a single triangle Point and Normal should be under the same index");
        }

        SUBCASE("Test error: Index out of range") {
            std::string out_of_range_string {R"(
v 0 0 0
vn 0 0 1
f 5/5/5 1/1/1 1/1/1)"};
            std::istringstream ss(out_of_range_string);
            auto result = mesh.read_mesh_from_obj(ss);
            
            CHECK(!result.has_value());
            CHECK(result.error() == "Problems with triangle indexing in .obj file: index out of range");
        }

        SUBCASE("Test valid stream") {
            std::string parsing_test_string {R"(
# Utah Teapot Model
o Utah Teapot
v -1.5 0 2.8
v -1.515625 -0.16875 2.753125
v -1.55 -0.225 2.65
vn 0 0 1
vn 0 -0.5999999 0.8000001
vn 0 -1 0
f 1/1/1 2/2/2 3/3/3)"};
            std::istringstream ss(parsing_test_string);
            CHECK(mesh.read_mesh_from_obj(ss).has_value());
            CHECK(mesh.mesh_points.size() == 3);
            CHECK(mesh.mesh_points[0].is_close(TrianglePoint{Point{-1.5f, 0.0f, 2.8f}, Normal{0.0f, 0.0f, 1.0f}}));
            CHECK(mesh.mesh_points[1].is_close(TrianglePoint{Point{-1.515625f, -0.16875f, 2.753125f}, Normal{0.0f, -0.5999999f, 0.8000001f}}));
            CHECK(mesh.mesh_points[2].is_close(TrianglePoint{Point{-1.55f, -0.225f, 2.65f}, Normal{0.0f, -1.0f, 0.0f}}));
            CHECK(mesh.triangle_points_indexes.size() == 1);
            CHECK(mesh.triangle_points_indexes[0].is_equal(TriangleIndexes{0, 1, 2})); // Indexes start from 1 in a .obj file, but in this module they start from 0
        }
    }

    SUBCASE("Test Mesh::read_mesh_from_obj(std::string)") {
        Mesh mesh;
        // Invalid file
        auto not_found_result = mesh.read_mesh_from_obj("Invalid_file.obj");
        CHECK(!not_found_result.has_value());
        CHECK(not_found_result.error() == "Can't find Invalid_file.obj");
        // Valid fine: a Utah teapot
        auto correct_file_path_result = mesh.read_mesh_from_obj("./mesh/utah_teapot.obj");
        CHECK(correct_file_path_result.has_value());
        // Check for the correct number of points
        CHECK(mesh.mesh_points.size() == 481);
        CHECK(mesh.triangle_points_indexes.size() == 880);
    }

    SUBCASE("Test Mesh::Mesh(...) (constructor from file name, applying BVH)") {
        // Utah teapot mesh
        Mesh mesh("./mesh/utah_teapot.obj");
        CHECK(mesh.mesh_points.size() == 481);
        CHECK(mesh.triangle_points_indexes.size() == 880);
//        for (const auto& node : mesh.nodes) {
//            if (node.is_leaf) {
//                // Recuperiamo la terzina di indici dei vertici di questo triangolo
//                const auto& tri_idx = mesh.triangle_points_indexes[node.minIndex];
//            
//                // Recuperiamo i 3 punti reali (punti + normali) dalla mesh
//                const auto& p1 = mesh.mesh_points[tri_idx.i1].point;
//                const auto& p2 = mesh.mesh_points[tri_idx.i2].point;
//                const auto& p3 = mesh.mesh_points[tri_idx.i3].point;
//            
//                // Stampiamo le coordinate X, Y, Z di ogni vertice del triangolo
//                std::println("Triangle: ({}, {}, {}) - ({}, {}, {}) - ({}, {}, {})",
//                    p1.x, p1.y, p1.z,
//                    p2.x, p2.y, p2.z,
//                    p3.x, p3.y, p3.z
//                );
//            }
//        }
//    }
}


    