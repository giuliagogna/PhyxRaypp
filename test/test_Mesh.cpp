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
static std::vector<Point> triangle_points;
static std::vector<Normal> triangle_normals;
static std::vector<Vec2D> texture_uv;
static std::vector<TriangleIndexes> triangle_point_indexes;
static std::vector<BVHNode> all_nodes;

TEST_CASE("TEST 2: BVHNode test suite") {
    
    // Generating 8 triangles placed on a cube vertices points

    triangle_points.push_back(Point{ 1.0f + 0.1f,  1.0f,  1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 0.0f});

    triangle_points.push_back(Point{ 1.0f + 0.1f,  1.0f, -1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 0.0f});
        
    triangle_points.push_back(Point{ 1.0f + 0.1f, -1.0f,  1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 0.0f});
        
    triangle_points.push_back(Point{ 1.0f + 0.1f, -1.0f, -1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 0.0f});
        
    triangle_points.push_back(Point{-1.0f + 0.1f,  1.0f,  1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 0.0f});
        
    triangle_points.push_back(Point{-1.0f + 0.1f,  1.0f, -1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 0.0f});
        
    triangle_points.push_back(Point{-1.0f + 0.1f, -1.0f,  1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 0.0f});
        
    triangle_points.push_back(Point{-1.0f + 0.1f, -1.0f, -1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 0.0f});
        

    triangle_points.push_back(Point{ 1.0f,  1.0f + 0.1f,  1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{1.0f, 0.0f});
        
    triangle_points.push_back(Point{ 1.0f,  1.0f + 0.1f, -1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{1.0f, 0.0f});
        
    triangle_points.push_back(Point{ 1.0f, -1.0f + 0.1f,  1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{1.0f, 0.0f});
        
    triangle_points.push_back(Point{ 1.0f, -1.0f + 0.1f, -1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{1.0f, 0.0f});
        
    triangle_points.push_back(Point{-1.0f,  1.0f + 0.1f,  1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{1.0f, 0.0f});
        
    triangle_points.push_back(Point{-1.0f,  1.0f + 0.1f, -1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{1.0f, 0.0f});
        
    triangle_points.push_back(Point{-1.0f, -1.0f + 0.1f,  1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{1.0f, 0.0f});
        
    triangle_points.push_back(Point{-1.0f, -1.0f + 0.1f, -1.0f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{1.0f, 0.0f});
        

    triangle_points.push_back(Point{ 1.0f,  1.0f,  1.0f + 0.1f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 1.0f});

    triangle_points.push_back(Point{ 1.0f,  1.0f, -1.0f + 0.1f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 1.0f});
        
    triangle_points.push_back(Point{ 1.0f, -1.0f,  1.0f + 0.1f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 1.0f});
        
    triangle_points.push_back(Point{ 1.0f, -1.0f, -1.0f + 0.1f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 1.0f});
        
    triangle_points.push_back(Point{-1.0f,  1.0f,  1.0f + 0.1f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 1.0f});
        
    triangle_points.push_back(Point{-1.0f,  1.0f, -1.0f + 0.1f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 1.0f});
        
    triangle_points.push_back(Point{-1.0f, -1.0f,  1.0f + 0.1f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 1.0f});
        
    triangle_points.push_back(Point{-1.0f, -1.0f, -1.0f + 0.1f});
    triangle_normals.push_back(Normal{0.0f, 0.0f, 1.0f});
    texture_uv.push_back(Vec2D{0.0f, 1.0f});

    // end of construction of triangles points, normals and UVs
        
    // Indicing
    triangle_point_indexes.push_back(TriangleIndexes{0,  8, 16, 0,  8, 16, 0,  8, 16});
    triangle_point_indexes.push_back(TriangleIndexes{1,  9, 17, 1,  9, 17, 1,  9, 17});
    triangle_point_indexes.push_back(TriangleIndexes{2, 10, 18, 2, 10, 18, 2, 10, 18});
    triangle_point_indexes.push_back(TriangleIndexes{3, 11, 19, 3, 11, 19, 3, 11, 19});
    triangle_point_indexes.push_back(TriangleIndexes{4, 12, 20, 4, 12, 20, 4, 12, 20});
    triangle_point_indexes.push_back(TriangleIndexes{5, 13, 21, 5, 13, 21, 5, 13, 21});
    triangle_point_indexes.push_back(TriangleIndexes{6, 14, 22, 6, 14, 22, 6, 14, 22});
    triangle_point_indexes.push_back(TriangleIndexes{7, 15, 23, 7, 15, 23, 7, 15, 23});    

    BVHAABB first_bounds;
    for (auto& point : triangle_points) {
        first_bounds.grow(point);
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
            std::make_shared<Material>(),
            triangle_points,
            triangle_normals,
            texture_uv,
            triangle_point_indexes,
            all_nodes
        };
        Ray test_ray{Point{1.0001f, 1.0001f, 2.0f}, Vec{0.0f, 0.0f, -1.0f}};
        auto record = mesh.ray_intersection(test_ray);
        CHECK(record.has_value());
        HitRecord hit_record = record.value();
        CHECK(hit_record.is_close(hit_record.hitted_shape->ray_intersection(test_ray).value()));
        CHECK(hit_record.hit_normal.is_close(Normal{0.0f, 0.0f, 1.0f}));
        CHECK(hit_record.hit_point.is_close(Point{1.0001f, 1.0001f, 1.1f}, 1e-3f));
        CHECK(aux::are_close(hit_record.t, 0.9f, 1e-3f));
        CHECK(hit_record.surface_params.is_close(Vec2D{0.0f, 1.0f}, 1e-2f));
    }

    SUBCASE("Test Mesh::ray_intersection with a Transformation") {
        Mesh mesh {
            Scale(Vec{1.0f, 1.0f, 2.0f}), 
            std::make_shared<Material>(), // <--- Sostituisci Material{} con questo
            triangle_points,
            triangle_normals,
            texture_uv,
            triangle_point_indexes,
            all_nodes
        };
        Ray test_ray{Point{1.0001f, 1.0001f, -3.0f}, Vec{0.0f, 0.0f, 1.0f}};
        auto record = mesh.ray_intersection(test_ray);
        CHECK(record.has_value());
        HitRecord hit_record = record.value();
        CHECK(hit_record.is_close(hit_record.hitted_shape->ray_intersection(test_ray).value()));
        CHECK(hit_record.hit_normal.is_close(Normal{0.0f, 0.0f, -1.0f}));
        CHECK(hit_record.hit_point.is_close(Point{1.0001f, 1.0001f, -1.8f}, 1e-3f));
        CHECK(aux::are_close(hit_record.t, 1.2f, 1e-3f));
        CHECK(hit_record.surface_params.is_close(Vec2D{0.0f, 1.0f}, 1e-2f));
    }

    SUBCASE("Test Mesh::read_mesh_from_obj(std::istream)") {
        Mesh mesh;

        SUBCASE("Test invalid stream (empty)") {
            std::istringstream empty_stream;
            auto stream_result = mesh.read_mesh_from_obj(empty_stream);
            CHECK(!stream_result.has_value());
            CHECK(stream_result.error() == "Problems with the .obj file");
        }

        SUBCASE("Test error: Index out of range") {
            std::string out_of_range_string {R"(
v 0 0 0
vn 0 0 1
vt 1 1 1
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
vt 1 1 and stuff that will be skipped
f 1/1/1 2/1/2 3/1/3)"};
            std::istringstream ss(parsing_test_string);
            CHECK(mesh.read_mesh_from_obj(ss).has_value());
            CHECK(mesh.mesh_points.size() == 3);
            CHECK(mesh.mesh_normals.size() == 3);
            CHECK(mesh.mesh_texture_uvs.size() == 1);
            CHECK(mesh.mesh_points[0].is_close(Point{-1.5f, 0.0f, 2.8f}));
            CHECK(mesh.mesh_normals[0].is_close(Normal{0.0f, 0.0f, 1.0f}));
            CHECK(mesh.mesh_points[1].is_close(Point{-1.515625f, -0.16875f, 2.753125f}));
            CHECK(mesh.mesh_normals[1].is_close(Normal{0.0f, -0.5999999f, 0.8000001f}));
            CHECK(mesh.mesh_points[2].is_close(Point{-1.55f, -0.225f, 2.65f}));
            CHECK(mesh.mesh_normals[2].is_close(Normal{0.0f, -1.0f, 0.0f}));
            CHECK(mesh.triangle_points_indexes.size() == 1);
            CHECK(mesh.triangle_points_indexes[0].is_equal(TriangleIndexes{0, 1, 2, 0, 1, 2, 0, 0, 0})); // Indexes start from 1 in a .obj file, but in this module they start from 0
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
    }
}


    