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

/**
 * @file Mesh.cppm
 * @brief 3D polygonal mesh representation and BVH acceleration.
 *
 * This module defines the Mesh shape used to load and render complex 3D models.
 * It handles parsing .obj files into an indexed face set architecture
 * and constructs a Bounding Volume Hierarchy (BVH) using the Surface Area
 * Heuristic (SAH) to highly accelerate ray-triangle intersection queries.
 */

module;

export module Mesh;
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

/**
 * @brief Axis-Aligned Bounding Box for spatial partitioning.
 *
 * Used to encapsulate geometry. If a ray misses the AABB, it guarantees the ray misses all
 * geometry contained within it, allowing for fast early rejection during BVH traversal.
 */
export struct BVHAABB {

    /**
     * @brief Minimum corner of the bounding box.
     * Stores the smallest coordinate along each axis (xmin, ymin, zmin).
     * Initialized to positive infinity so that the first call to grow() correctly expands the box.
     */
    Point minPoint{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};

    /**
     * @brief Maximum corner of the bounding box.
     * Stores the largest coordinate along each axis (xmax, ymax, zmax).
     * Initialized to negative infinity so that the first call to grow() correctly expands the box.
     */
    Point maxPoint{-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};

    /**
     * @brief Expands the bounding box to include a point.
     * Updates minPoint and maxPoint so that the resulting bounding box contains both its previous volume and p.
     * @param p Point to include.
     */
    void grow(const Point& p) {
        minPoint = min(minPoint, p);
        maxPoint = max(maxPoint, p);
    }

    /**
     * @brief Expands the bounding box to include another bounding box.
     * Computes the union of the two AABBs.
     * @param b Bounding box to include.
     */
    void grow(const BVHAABB& b) {
        minPoint = min(minPoint, b.minPoint);
        maxPoint = max(maxPoint, b.maxPoint);
    }

    /**
     * @brief Computes the surface area of the bounding box.
     *
     * The surface area is used by the Surface Area Heuristic (SAH) during BVH construction to estimate split quality.
     *
     * @return Surface area of the box.
     */
    float area() const {
        Vec e = maxPoint - minPoint;
        return 2.0f * (e.x * e.y + e.y * e.z + e.z * e.x);
    }

    /**
     * @brief Returns the longest axis of the bounding box.
     *
     * Commonly used during BVH construction to choose a splitting direction for spatial partitioning.
     *
     * @return
     * - 0 : X axis
     * - 1 : Y axis
     * - 2 : Z axis
     */
    int longestAxis() const {
        Vec e = maxPoint - minPoint;
        if (e.x > e.y && e.x > e.z) return 0;
        return e.y > e.z ? 1 : 2;
    }

    /**
     * @brief Tests whether a ray intersects the bounding box.
     *
     * Uses the slab intersection algorithm, computing the entry and exit intervals along each coordinate
     * axis and combining them into a single valid intersection range.
     *
     * The implementation employs precomputed inverse ray directions and sign checks
     * to minimize branching during traversal.
     *
     * @param ray Ray to test.
     *
     * @return
     * True if the ray intersects the box within the interval [ray.tmin, ray.tmax],
     * false otherwise.
     */
    bool intersect(const Ray& ray) const {
        Vec inv_direction{1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z};

        // Sign logic to avoid if()
        Point bounds[2] = {minPoint, maxPoint};
        int sign[3] = {
            (ray.direction.x < 0.0f),
            (ray.direction.y < 0.0f),
            (ray.direction.z < 0.0f)
        };

        float tmin = (bounds[sign[0]].x - ray.origin.x) * inv_direction.x;
        float tmax = (bounds[1 - sign[0]].x - ray.origin.x) * inv_direction.x;

        float tymin = (bounds[sign[1]].y - ray.origin.y) * inv_direction.y;
        float tymax = (bounds[1 - sign[1]].y - ray.origin.y) * inv_direction.y;

        tmin = std::max(tmin, std::min(tymin, tymax));
        tmax = std::min(tmax, std::max(tymin, tymax));

        float tzmin = (bounds[sign[2]].z - ray.origin.z) * inv_direction.z;
        float tzmax = (bounds[1 - sign[2]].z - ray.origin.z) * inv_direction.z;

        tmin = std::max(tmin, std::min(tzmin, tzmax));
        tmax = std::min(tmax, std::max(tzmin, tzmax));

        // Hit check
        return (tmin < tmax && tmax > ray.tmin && tmin < ray.tmax);
    }
};

/**
 * @brief Index triplet describing a triangular face in an indexed mesh.
 *
 * Stores the indices of the three vertices composing a triangle, along with the corresponding normal and
 * texture-coordinate indices as defined by the .obj format.
 *
 * All indices refer to elements stored in the Mesh vertex, normal, and UV arrays.
 */
export struct TriangleIndexes {

    /// Index of the first vertex position.
    int v1;

    /// Index of the second vertex position.
    int v2;

    /// Index of the third vertex position.
    int v3;

    /// Index of the normal associated with the first vertex.
    int vn1;

    /// Index of the normal associated with the second vertex.
    int vn2;

    /// Index of the normal associated with the third vertex.
    int vn3;

    /// Index of the texture coordinate associated with the first vertex.
    int vt1;

    /// Index of the texture coordinate associated with the second vertex.
    int vt2;

    /// Index of the texture coordinate associated with the third vertex.
    int vt3;

    /**
     * @brief Compares two triangle index sets for exact equality.
     *
     * @param other Triangle index set to compare against.
     * @return True if all vertex, normal, and texture-coordinate indices match.
     */
    bool is_equal(const TriangleIndexes& other) {
        return (v1 == other.v1 && v2 == other.v2 && v3 == other.v3 &&
                vn1 == other.vn1 && vn2 == other.vn2 && vn3 == other.vn3 &&
                vt1 == other.vt1 && vt2 == other.vt2 && vt3 == other.vt3);
    }
};

/**
 * @brief Temporary bin used during BVH construction.
 *
 * Stores the accumulated bounding box and triangle count for a spatial
 * partition generated by the SAH binning algorithm.
 */
export struct BVHBin {

     /// Bounding box enclosing all triangles assigned to this bin.
    BVHAABB bounds;

    /// Number of triangles currently assigned to this bin.
    int trianglesCount = 0;
};

/**
 * @brief Returns a point coordinate along a selected axis.
 *
 * Utility helper used by the BVH construction algorithm when sorting
 * or binning primitives along a specific dimension.
 *
 * @param p Input point.
 * @param axis Coordinate axis:
 *             - 0 = X
 *             - 1 = Y
 *             - 2 = Z
 * @return The coordinate value corresponding to the selected axis.
 */
inline float get_axis_value(const Point& p, int axis) {
    switch (axis) {
        case 0: return p.x;
        case 1: return p.y;
        default: return p.z;
    }
}

/**
 * @brief Node of the Bounding Volume Hierarchy (BVH) Tree.
 *
 * Each node stores an axis-aligned bounding box enclosing a contiguous range
 * of triangles from the mesh. Internal nodes contain references to two child
 * nodes, while leaf nodes directly represent a small subset of triangles.
 *
 * The BVH is stored as a flat std::vector<BVHNode>, therefore child references
 * are represented by integer indices rather than pointers.
 */
export struct BVHNode {

    /// Bounding box enclosing all triangles assigned to this node.
    BVHAABB bounds;

    /// Index of the left child node in the BVH node vector.
    /// Equal to -1 if this node is a leaf.
    int left_child_index = -1;

    /// Index of the right child node in the BVH node vector.
    /// Equal to -1 if this node is a leaf.
    int right_child_index = -1;

    /// Position of this node inside the BVH node vector.
    int node_index = -1;

    /**
     * @brief First triangle index belonging to this node.
     *
     * Refers to the triangle_point_indexes array.
     * The node owns triangles in the half-open range [minIndex, maxIndex).
     */
    int minIndex = 0;

    /**
     * @brief One-past-the-end triangle index belonging to this node.
     *
     * Together with minIndex defines the half-open range [minIndex, maxIndex).
     */
    int maxIndex = 0;

    /// True if this node is a leaf and contains no child nodes.
    bool is_leaf = false;

    /**
     * @brief Builds the BVH subtree rooted at this node.
     *
     * Public entry point used to recursively construct the BVH.
     * This wrapper automatically selects the optimal initial splitting axis and forwards
     * the work to the internal recursive builder.
     *
     * The resulting hierarchy is generated using a Surface Area
     * Heuristic (SAH) binning strategy.
     *
     * @param current_nodes Flat vector containing all BVH nodes.
     * @param mesh_points Vertex positions of the mesh.
     * @param triangle_point_indexes Triangle index buffer to partition.
     * @param n_bins Number of bins used by the SAH splitting algorithm.
     * @param is_leaf_threshold Maximum number of triangles allowed in a leaf node.
     */
    void Extend_tree_wrapper(
        std::vector<BVHNode>& current_nodes,
        const std::vector<Point>& mesh_points,
        std::vector<TriangleIndexes>& triangle_point_indexes,
        const int n_bins,
        const int is_leaf_threshold = 1) {
    
        Extend_tree(
            current_nodes,
            current_nodes.size() - 1,
            mesh_points,
            triangle_point_indexes,
            n_bins,
            is_leaf_threshold);
    }
private:
    /**
     * @brief Recursively constructs a BVH subtree using SAH binning.
     *
     * The algorithm:
     *  - Computes triangle centroids for the current node.
     *  - Selects a splitting axis.
     *  - Partitions triangles into spatial bins.
     *  - Evaluates candidate split planes using the Surface Area Heuristic.
     *  - Creates child nodes if subdivision reduces traversal cost.
     *  - Recursively repeats the process for each child.
     *
     * The BVH is stored as a flat vector. Node indices are therefore used
     * instead of pointers to remain valid even if vector reallocations occur.
     *
     * @param current_nodes Container storing the entire BVH.
     * @param node_index Index of the node currently being subdivided.
     * @param mesh_points Vertex positions of the mesh.
     * @param triangle_point_indexes Triangle index buffer being partitioned.
     * @param n_bins Number of SAH bins used to evaluate split candidates.
     * @param is_leaf_threshold Maximum number of triangles allowed in a leaf.
     * @param axis Splitting axis to test. Values outside [0,2] trigger
     *             automatic selection of the longest centroid axis.
     */
    static void Extend_tree(
        std::vector<BVHNode>& current_nodes,
        int node_index,const std::vector<Point>& mesh_points,
        std::vector<TriangleIndexes>& triangle_point_indexes,
        const int n_bins,
        const int is_leaf_threshold = 1,
        int axis = 3) {

        // Store the array index explicitly because child nodes are referenced by integer indices rather than pointers.
        current_nodes[node_index].node_index = node_index;
        int minIndex = current_nodes[node_index].minIndex;
        int maxIndex = current_nodes[node_index].maxIndex;

        // Stop if the first node is a leaf (contains isLeaf_threshold or less triangles).
        if (maxIndex - minIndex <= is_leaf_threshold) {
            current_nodes[node_index].is_leaf = true;
            return;
        }

        // Compute each triangle's centroid to determine the BVHAABB size of centroids only
        BVHAABB centroid_bounds;
        for (int i = minIndex; i < maxIndex; ++i) {
            const auto& tri = triangle_point_indexes[i];

            Point centroid = (mesh_points[tri.v1] +
                              mesh_points[tri.v2].to_vec() +
                              mesh_points[tri.v3].to_vec()) * (1.0f / 3.0f);

            centroid_bounds.grow(centroid);
        }

        // Can't split if the mesh is a funny ensemble of triangles with the same centroid
        if (centroid_bounds.maxPoint.is_close(centroid_bounds.minPoint)) {
            current_nodes[node_index].is_leaf = true;
            return;
        }

        // First call: compute the best axis and use it
        if (axis > 2 || axis < 0) {
            axis = centroid_bounds.longestAxis();
        }

        // -----------------------------------
        // Binning
        // -----------------------------------

        // Setup bins and calculate the multiplier used to map 3D distances directly to array indices
        std::vector<BVHBin> bins(n_bins);
        float scale = n_bins / (get_axis_value(centroid_bounds.maxPoint, axis) - get_axis_value(centroid_bounds.minPoint, axis)); // Inverse of the bin dimension

        // Select each bin for each triangle based on its centroid
        for (int i = minIndex; i < maxIndex; ++i) {
            Point centroid = (mesh_points[triangle_point_indexes[i].v1] +
                              mesh_points[triangle_point_indexes[i].v2].to_vec() +
                              mesh_points[triangle_point_indexes[i].v3].to_vec()) * (1.0f / 3.0f);

            // Find the bin index where the triangle belongs to
            // - subtract the start of the box from the triangle's position to find out "how far into the box" the triangle is.
            // - multiply that distance by the scale.
            // - static_cast<int> chops off any decimals so we get a clean integer array index.
            // - clamp: std::max(0, std::min(n_bins - 1, ...)) forces the index to stay safely between 0 and 11 to prevent a crash.
            int bin_index = std::max(0,
                                     std::min(n_bins - 1,
                                                 static_cast<int>((get_axis_value(centroid, axis) - get_axis_value(centroid_bounds.minPoint, axis)) * scale)
                                                )
                                     );

            // Update the population of the bin
            bins[bin_index].trianglesCount++;
            
            // Grow the AABB of the bin to encompass all three vertexes of the new triangle
            bins[bin_index].bounds.grow(mesh_points[triangle_point_indexes[i].v1]);
            bins[bin_index].bounds.grow(mesh_points[triangle_point_indexes[i].v2]);
            bins[bin_index].bounds.grow(mesh_points[triangle_point_indexes[i].v3]);
        }

        // -------------------------------------------------------
        // Surface Area Heuristic (SAH): decide where to split
        // -------------------------------------------------------
        // SAH formula (to be minimized):
        // C ~ AreaL * CountL + AreaR * CountR (simplified formula neglecting tree branching overhead)    

        // Compute the left term values
        // Indexes run forward: counting from left to right.
        std::vector<float> left_bounds_area(n_bins - 1);
        std::vector<int> left_count(n_bins - 1);
        int left_sum = 0; 
        BVHAABB left_box;
        for (int i = 0; i < n_bins - 1; ++i) {
            left_sum += bins[i].trianglesCount; // Cumulative count of triangles from the left
            left_box.grow(bins[i].bounds); // Grow the AABB from the left
            left_bounds_area[i] = left_box.area(); // Compute the area at this point
            left_count[i] = left_sum; // Count the triangles 
        }

        // Compute the right term.
        // Indexes run backwards: counting from right to left.
        // - right-hand arrays are saved into index i - 1: when you are at split i,
        //   the left side includes bins 0 to i, and the right side includes bins i+1 to the end.
        std::vector<float> right_bounds_area(n_bins - 1);
        std::vector<int> right_count(n_bins - 1);
        BVHAABB right_box;
        int right_sum = 0;
        for (int i = n_bins - 1; i > 0; --i) {
            right_sum += bins[i].trianglesCount;
            right_box.grow(bins[i].bounds);
            right_bounds_area[i - 1] = right_box.area();
            right_count[i - 1] = right_sum;
        }

        // Compute split costs
        float best_cost = std::numeric_limits<float>::infinity();
        float split_value = 0.0f;
        float bin_width = (get_axis_value(centroid_bounds.maxPoint, axis) -
                             get_axis_value(centroid_bounds.minPoint, axis)) / n_bins;

        for (int i = 0; i < n_bins - 1; ++i) {
            float cost = left_bounds_area[i] * left_count[i] + right_bounds_area[i] * right_count[i];
            if (cost < best_cost) {
                best_cost = cost;
                split_value = get_axis_value(centroid_bounds.minPoint, axis) + (i + 1) * bin_width;
            }
        }

        // No split cost: calculate the cost keeping all the triangles in the current node
        // - (maxIndex - minIndex) is the number of triangles in the current box
        // If no splitting costs less than best splitting, the node is no more splitted and considered a leaf
        float no_split_cost = current_nodes[node_index].bounds.area() * (maxIndex - minIndex);
        if (best_cost >= no_split_cost) {
            current_nodes[node_index].is_leaf = true;
            return;
        }

        // Now we have the split_value to cut the current AABB

        // -------------------------------------------------------
        // Actual splitting
        // -------------------------------------------------------
        // Reorder the std::vector segment to have the left child members at the start of the window
        // and the right child members at the end

        // This will be the max index of the left child, so it will be the first index of the right child
        int left_maxIndex = minIndex;

        // Triangle partition: scoped so i and j only exist in the partition phase
        {
            int i = minIndex;
            int j = maxIndex - 1;

            while (i <= j) {
                Point centroid = (mesh_points[triangle_point_indexes[i].v1] +
                                  mesh_points[triangle_point_indexes[i].v2].to_vec() +
                                  mesh_points[triangle_point_indexes[i].v3].to_vec()) * (1.0f / 3.0f);

                if (get_axis_value(centroid, axis) < split_value) {
                    i++;
                } else {
                    std::swap(triangle_point_indexes[i], triangle_point_indexes[j]);
                    j--;
                    // "i" does not move, so now it contains a different element that needs to be checked
                }
            }
            // At termination [minIndex, i) contains all the triangles with centroid < split_value
            // and [i, maxIndex) contains all the triangles with centroid >= split_value
            left_maxIndex = i;
        }

        // If the triangles were not actually split (left or right child is empty and the other has all the triangles)
        // check if there is a better cutting for the AABB along other axis
        if (left_maxIndex == minIndex || left_maxIndex == maxIndex) {

            // Cycle on axis indexes
            axis = (axis + 1) % 3;

            // When the cycle gets back to the longest axis, then all the axis have failed, the node is
            // not splittable, therefore it is a leaf
            if (axis == current_nodes[node_index].bounds.longestAxis()) {
                current_nodes[node_index].is_leaf = true;
                return;

            } else { // Try with another direction
                Extend_tree(current_nodes, node_index, mesh_points, triangle_point_indexes, n_bins, is_leaf_threshold, axis);
                return;
            }
        }


        // Allocate the new children in the std::vector<BVHNode>
        current_nodes[node_index].left_child_index = static_cast<int>(current_nodes.size());
        current_nodes.push_back(BVHNode());
        current_nodes[node_index].right_child_index = static_cast<int>(current_nodes.size());
        current_nodes.push_back(BVHNode());

        // Update data-members of the children
        // Triangle points indexes of the child
        current_nodes[current_nodes[node_index].left_child_index].minIndex = minIndex;
        current_nodes[current_nodes[node_index].left_child_index].maxIndex = left_maxIndex;
        current_nodes[current_nodes[node_index].right_child_index].minIndex = left_maxIndex;
        current_nodes[current_nodes[node_index].right_child_index].maxIndex = maxIndex;

        // Update the AABB of the children
        // Build the left child's AABB
        current_nodes[current_nodes[node_index].left_child_index].bounds = BVHAABB();
        // Expand the box until it contains all vertexes
        for (int i = current_nodes[current_nodes[node_index].left_child_index].minIndex; i < current_nodes[current_nodes[node_index].left_child_index].maxIndex; ++i) {
            current_nodes[current_nodes[node_index].left_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].v1]);
            current_nodes[current_nodes[node_index].left_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].v2]);
            current_nodes[current_nodes[node_index].left_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].v3]);
        }
        // Build the right child's AABB
        current_nodes[current_nodes[node_index].right_child_index].bounds = BVHAABB();
        // Expand the box until it contains all vertexes
        for (int i = current_nodes[current_nodes[node_index].right_child_index].minIndex; i < current_nodes[current_nodes[node_index].right_child_index].maxIndex; ++i) {
            current_nodes[current_nodes[node_index].right_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].v1]);
            current_nodes[current_nodes[node_index].right_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].v2]);
            current_nodes[current_nodes[node_index].right_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].v3]);
        }

        // Call this same method recursively to generate the entire tree recursively
        current_nodes[current_nodes[node_index].left_child_index].Extend_tree(current_nodes, current_nodes[node_index].left_child_index, mesh_points, triangle_point_indexes, n_bins, is_leaf_threshold);
        current_nodes[current_nodes[node_index].right_child_index].Extend_tree(current_nodes, current_nodes[node_index].right_child_index, mesh_points, triangle_point_indexes, n_bins, is_leaf_threshold);

        return;
    }    
};


// ====================================================================================================
// ====================================================================================================


// ====================
// POLYGONAL MESH
// ====================

/**
 * @brief Polygonal mesh accelerated by a Bounding Volume Hierarchy (BVH).
 *
 * A Mesh stores geometry loaded from an indexed triangle representation (typically parsed from a .obj file).
 * Triangle intersection queries are accelerated through a BVH tree built using the Surface Area Heuristic (SAH).
 *
 * The mesh inherits from Shape, allowing it to participate in the scene
 * exactly like built-in primitives such as spheres, planes, and cubes.
 */
export struct Mesh : Shape {

    /// @brief Vertex positions of the mesh.
    /// Corresponds to the 'v' records in the .obj file.
    /// TriangleIndexes::v1, v2 and v3 reference elements of this array.
    std::vector<Point> mesh_points;

    /// @brief Per-vertex normal vectors.
    /// Corresponds to the 'vn' records in the .obj file.
    /// TriangleIndexes::vn1, vn2 and vn3 reference elements of this array.
    std::vector<Normal> mesh_normals;

    /// @brief Texture coordinates associated with mesh vertices.
    /// Corresponds to the 'vt' records in the .obj file. Used during texture lookup and UV interpolation.
    std::vector<Vec2D> mesh_texture_uvs;

    /// @brief Indexed triangle list describing mesh connectivity.
    /// Each entry stores indices into the vertex, normal and UV arrays, defining one triangular face of the mesh.
    std::vector<TriangleIndexes> triangle_points_indexes;

    /// @brief Flat storage of the Bounding Volume Hierarchy.
    /// The BVH is stored as a contiguous array of nodes rather than as a pointer-based tree.
    /// Child nodes are referenced through the indices stored in BVHNode::left_child_index and BVHNode::right_child_index.
    std::vector<BVHNode> nodes;

    using Shape::Shape;

    /// Default constructor added for compatibility with some compilers
    Mesh() = default;

    /**
     * @brief Constructs a mesh from pre-built geometry and BVH data.
     *
     * This constructor is mainly intended for internal use, testing, serialization, or
     * procedural mesh generation where all geometry and acceleration structures are already available.
     *
     * @param trans Object-to-world transformation.
     * @param material Material applied to the mesh surface.
     * @param points Vertex positions.
     * @param normals Vertex normals.
     * @param texture_uvs Texture coordinates.
     * @param indexes Indexes Triangle mesh faces.
     * @param bvh_nodes Precomputed BVH nodes.
     */
    Mesh(
        const Transformation& trans,
        std::shared_ptr<Material> material,
        std::vector<Point> points,
        std::vector<Normal> normals,
        std::vector<Vec2D> texture_uvs,
        std::vector<TriangleIndexes> indexes,
        std::vector<BVHNode> bvh_nodes) : Shape(trans, material),
                                          mesh_points(std::move(points)),
                                          mesh_normals(std::move(normals)),
                                          mesh_texture_uvs(texture_uvs),
                                          triangle_points_indexes(std::move(indexes)),
                                          nodes(std::move(bvh_nodes)) {}

    /**
     * @brief Loads a mesh from an .obj file and builds its BVH.
     *
     * The constructor parses the geometry contained in the specified file, initializes the mesh data structures,
     * creates the root bounding box, and recursively builds a Bounding Volume Hierarchy using the
     * Surface Area Heuristic via BVHNode::Extend_tree_wrapper.
     *
     * @param obj_file Path to the .obj file.
     * @param material Material assigned to the mesh.
     * @param trans Object-to-world transformation.
     * @param BVH_n_bins Number of bins used by the SAH binning algorithm.
     *                   Higher values may improve BVH quality at the cost of longer construction times.
     * @param BVH_is_leaf_threshold Minimum number of triangles required before subdivision is attempted.
     */
    Mesh(
        std::string obj_file,
        std::shared_ptr<Material> material = nullptr,
        Transformation trans = Transformation{},
        int BVH_n_bins = 12,
        int BVH_is_leaf_threshold = 3) : Shape(trans, material) {

        // Load the .obj file and check for errors
        auto file_reading_result = read_mesh_from_obj(obj_file);
        if (!file_reading_result.has_value()) {
            std::println("ERROR: ");
            std::print("{}", file_reading_result.error());
            std::print("\n --- from constructor Mesh::Mesh(std::string, std::shared_ptr<Material>)");
            return;

        }

        // Create the root node for BVH
        BVHNode root;
        root.minIndex = 0;
        root.maxIndex = triangle_points_indexes.size();

        // Extend AABB of the root
        for (auto& mesh_point : mesh_points) {
            root.bounds.grow(mesh_point);
        }
        nodes.push_back(root);
        // Call BVHNode::Extend_tree via wrapper
        nodes[0].Extend_tree_wrapper(nodes, mesh_points, triangle_points_indexes, BVH_n_bins, BVH_is_leaf_threshold);
    }

    // -------------------------------------------------------------
    // RAY INTERSECTION (Möller–Trumbore algorithm)
    // -------------------------------------------------------------

    /**
     * @brief Interface entry point for ray-mesh intersection.
     *
     * Transforms the incoming ray from world space into the mesh local reference frame,
     * delegates the intersection search to ray_intersection_unwrapped(), and converts the resulting hit
     * information back into world coordinates.
     *
     * @param ray Incoming world-space ray.
     * @return The closest intersection with the mesh, or std::nullopt if
     *         the ray misses the mesh entirely.
     */
    [[nodiscard]] std::optional<HitRecord> ray_intersection(const Ray& ray) const override {

        // Transform the ray in the reference frame of the mesh
        Ray local_ray = ray.transform(trans.inverse());

        std::optional<HitRecord> closest_hit = std::nullopt;
        auto hit_record = ray_intersection_unwrapped(local_ray, closest_hit);
        if (hit_record.has_value()) {
            hit_record->ray = ray;
            hit_record->hit_point = ray.at(hit_record->t);
            hit_record->hit_normal = (trans * hit_record->hit_normal).normalize();
        }
        return hit_record;

    }

    /**
     * @brief Recursively traverses the BVH to find the closest ray-triangle intersection.
     *
     * This method operates entirely in the mesh local coordinate system.
     *
     * Starting from the BVH node identified by starting_index:
     * - the node AABB is tested against the ray;
     * - if the ray misses the box, the subtree is skipped;
     * - if the node is a leaf, every contained triangle is tested using the Möller–Trumbore ray-triangle intersection algorithm;
     * - if the node is an internal node, the method recursively visits both children.
     *
     * When a triangle intersection is found:
     * - barycentric coordinates are computed;
     * - vertex normals are interpolated using those barycentric weights to obtain a smooth shading normal;
     * - texture coordinates are interpolated in the same way;
     * - a partial HitRecord is generated.
     *
     * Whenever a closer intersection is found, local_ray.tmax is reduced to the hit distance, allowing subsequent tests
     * to ignore farther geometry and improving traversal performance.
     *
     * The returned HitRecord is expressed in the mesh local reference frame.
     * World-space information such as the final hit point, transformed normal, and original ray are
     * completed by ray_intersection().
     *
     * @param local_ray Ray expressed in mesh local coordinates.
     *                  The ray is modified during traversal by shrinking
     *                  its tmax value whenever a closer hit is found.
     * @param starting_index Index of the BVH node currently being explored.
     *                       Defaults to the root node (0).
     * @param closest_hit Current closest intersection discovered so far. It's passed by reference to avoid unnecessary copies.
     *
     * @return The nearest HitRecord found in the subtree rooted at
     *         starting_index, or std::nullopt if no intersection exists.
     */
    [[nodiscard]] std::optional<HitRecord> ray_intersection_unwrapped(
        Ray& local_ray,
        std::optional<HitRecord>& closest_hit,
        int starting_index = 0) const {

        // If the ray misses this box, skip it
        if (!nodes[starting_index].bounds.intersect(local_ray)) {
            return closest_hit; // No intersection with the bounding box, skip this node
        }

        // If the node is a leaf check the intersection of the ray with the triangles in the node
        if (nodes[starting_index].is_leaf) {
            int minIndex = nodes[starting_index].minIndex;
            int maxIndex = nodes[starting_index].maxIndex;

            // Loop on the triangles
            for (int i = minIndex; i < maxIndex; ++i) {

                const auto& tri = triangle_points_indexes[i];

                const auto& A = mesh_points[tri.v1];
                const auto& B = mesh_points[tri.v2];
                const auto& C = mesh_points[tri.v3];

                // Ray-triangle intersection test (Möller–Trumbore algorithm)
                Vec E1 = B - A;
                Vec E2 = C - A;

                Vec P = local_ray.direction % E2;
                Vec T = local_ray.origin - A;
                Vec Q = T % E1;

                // Determinant calculation
                float det = E1 * P;
                // Early exit if ray is parallel or nearly parallel to the triangle plane (determinant=0)
                if (std::abs(det) < 1e-6f) {
                    continue;
                }

                float inv_det = 1.0f / det;

                // Calculate the barycentric coordinates alpha, beta, gamma
                float beta = (T * P) * inv_det;
                float gamma = (Q * local_ray.direction) * inv_det;
                float alpha = 1.0f - beta - gamma;

                float t = (Q * E2) * inv_det;

                if (beta >= 0.0f && gamma >= 0.0f && alpha > 0.0f && alpha <= 1.0f && t >= local_ray.tmin && t <= local_ray.tmax) {

                    auto& nA = mesh_normals[tri.vn1];
                    auto& nB = mesh_normals[tri.vn2];
                    auto& nC = mesh_normals[tri.vn3];
                    auto& tA = mesh_texture_uvs[tri.vt1];
                    auto& tB = mesh_texture_uvs[tri.vt2];
                    auto& tC = mesh_texture_uvs[tri.vt3];

                    // Some members of the HitRecord struct will be updated outside this method,
                    // since we are in the frame of reference of the Mesh.

                    HitRecord hit;
                    hit.t = t;
                    Vec interpolated_normal = (nA * alpha + nB * beta + nC * gamma);
                    // Use std::copysign to flip normal if the ray comes from inside
                    hit.hit_normal = (interpolated_normal * (-std::copysign(1.0f, interpolated_normal * local_ray.direction))).to_norm().normalize(); // Raw conversion to_norm() needs to be normalized to ensure unit length
                    hit.surface_params = {alpha * tA.u + beta * tB.u + gamma * tC.u,
                                             alpha * tA.v + beta * tB.v + gamma * tC.v};
                    hit.hitted_shape = this; // Point directly to this exact mesh in memory
                    // Update local ray tmax
                    local_ray.tmax = t; // Shrink the ray range to find closer intersections
                    closest_hit = hit; // Update the closest hit
                }
            }

            return closest_hit;

        } else { // If the current node is not a leaf, call the method recursively on the children

            if (auto left_hit = ray_intersection_unwrapped(local_ray, closest_hit, nodes[starting_index].left_child_index)) {
                local_ray.tmax = left_hit->t; // Shrink the ray range to find closer intersections
                closest_hit = left_hit;
            }

            if (auto right_hit = ray_intersection_unwrapped(local_ray, closest_hit, nodes[starting_index].right_child_index)) {
                local_ray.tmax = right_hit->t; // Shrink the ray range to find closer intersections
                closest_hit = right_hit; // Guaranteed to be closer if found because local_ray.tmax was shrunk
            }

            return closest_hit; // Return the closest hit found in the children

        }
    }

    /**
     * @brief Recursively traverses the BVH to find ALL ray-triangle intersections.
     *
     * Unlike the standard intersection method, this traversal does NOT shrink 
     * local_ray.tmax. It must explore every node intersected by the ray.
     */
    void ray_all_intersections_unwrapped(
        const Ray& local_ray, // Passed as const! We do NOT shrink tmax here.
        int starting_index,
        std::vector<directionalHitRecord>& hits_collection) const {

        // If the ray misses this box, skip it
        if (!nodes[starting_index].bounds.intersect(local_ray)) {
            return; 
        }

        // If the node is a leaf check the intersection of the ray with the triangles in the node
        if (nodes[starting_index].is_leaf) {
            int minIndex = nodes[starting_index].minIndex;
            int maxIndex = nodes[starting_index].maxIndex;

            // Loop on the triangles
            for (int i = minIndex; i < maxIndex; ++i) {

                const auto& tri = triangle_points_indexes[i];

                const auto& A = mesh_points[tri.v1];
                const auto& B = mesh_points[tri.v2];
                const auto& C = mesh_points[tri.v3];

                // Ray-triangle intersection test (Möller–Trumbore algorithm)
                Vec E1 = B - A;
                Vec E2 = C - A;

                Vec P = local_ray.direction % E2;
                Vec T = local_ray.origin - A;
                Vec Q = T % E1;

                float det = E1 * P;
                if (std::abs(det) < 1e-6f) {
                    continue;
                }

                float inv_det = 1.0f / det;

                // Calculate the barycentric coordinates alpha, beta, gamma
                float beta = (T * P) * inv_det;
                float gamma = (Q * local_ray.direction) * inv_det;
                float alpha = 1.0f - beta - gamma;

                float t = (Q * E2) * inv_det;

                // Notice we still check against local_ray.tmin and local_ray.tmax, 
                // but those bounds remain constant throughout the traversal.
                if (beta >= 0.0f && gamma >= 0.0f && alpha > 0.0f && alpha <= 1.0f && t >= local_ray.tmin && t <= local_ray.tmax) {

                    auto& nA = mesh_normals[tri.vn1];
                    auto& nB = mesh_normals[tri.vn2];
                    auto& nC = mesh_normals[tri.vn3];
                    auto& tA = mesh_texture_uvs[tri.vt1];
                    auto& tB = mesh_texture_uvs[tri.vt2];
                    auto& tC = mesh_texture_uvs[tri.vt3];

                    Vec interpolated_normal = (nA * alpha + nB * beta + nC * gamma);

                    // Entry/Exit evaluation based on the true geometric normal
                    float dot = interpolated_normal * local_ray.direction;
                    bool is_entering = (dot < 0.0f);

                    // Branchless normal flip
                    float flip = -std::copysign(1.0f, dot);

                    directionalHitRecord hit;
                    hit.t = t;
                    hit.hit_normal = (interpolated_normal * flip).to_norm();
                    hit.surface_params = {
                        alpha * tA.u + beta * tB.u + gamma * tC.u,
                        alpha * tA.v + beta * tB.v + gamma * tC.v
                    };
                    hit.hitted_shape = this;
                    hit.is_entering = is_entering;

                    // Accumulate the intersection without shrinking the ray
                    hits_collection.push_back(hit);
                }
            }
        } else { 
            // If the current node is not a leaf, call the method recursively on the children
            // We must visit BOTH children if their AABB was hit (checked at the start of the call)
            ray_all_intersections_unwrapped(local_ray, nodes[starting_index].left_child_index, hits_collection);
            ray_all_intersections_unwrapped(local_ray, nodes[starting_index].right_child_index, hits_collection);
        }
    }

    /**
     * @brief Interface entry point for finding all ray-mesh intersections.
     *
     * Similar to ray_intersection, but collects every valid intersection along the ray
     * without shrinking the ray's tmax. The final hits are transformed back to world
     * space.
     */
    [[nodiscard]] std::vector<directionalHitRecord> ray_all_intersections(const Ray& ray) const override {
        // vector to be filled of intersections
        std::vector<directionalHitRecord> intersections;

        // Transform the ray in the reference frame of the mesh
        Ray local_ray = ray.transform(trans.inverse());

        // Start recursion flow with transformed ray. 
        // Pass the vector by reference to collect all hits.
        ray_all_intersections_unwrapped(local_ray, 0, intersections);

        // Transform everything back to world coordinates
        for (auto& record : intersections) {
            record.ray = ray;
            record.hit_point = ray.at(record.t); 
            record.hit_normal = (trans * record.hit_normal).normalize();
        }

        return intersections;
    }



    // -------------------------------------------------------
    // READING UTILITIES
    // -------------------------------------------------------

    /**
     * @brief Reads the next meaningful line from a text stream.
     *
     * Empty lines and comment lines beginning with '#' are skipped.
     * Trailing carriage-return characters ('\r') are removed to support Windows-formatted text files.
     *
     * @param stream Input stream.
     * @return The next non-empty, non-comment line, or an error if
     *         no such line can be read.
     */
    [[nodiscard]] std::expected<std::string, std::string> _read_line(std::istream& stream) {
        std::string result;

        // read till we find a line that is not a comment (starting with #) and not empty (for example an extra newline)
        while (std::getline(stream, result)) {
            // remove carriage return \r
            if (!result.empty() && result.ends_with("\r")) {
                result.pop_back();
            }

            // Skip empty lines
            if (result.empty())
                continue;

            // Skip comment lines
            if (result.starts_with('#'))
                continue;

            return result;
        }

        return std::unexpected("Impossible to read line.");
    }

    /**
     * @brief Loads mesh geometry from an obj stream.
     *
     * The parser supports:
     * - vertex positions ("v")
     * - vertex normals ("vn")
     * - texture coordinates ("vt")
     * - polygonal faces ("f")
     *
     * Faces containing more than three vertices are automatically triangulated using
     * a fan triangulation strategy.
     *
     * Parsed geometry is stored in the mesh vertex, normal, texture, and triangle index arrays.
     *
     * @param obj_stream Stream containing OBJ file contents.
     * @return Success or an error message if the file format is invalid.
     */
    [[nodiscard]] std::expected<void, std::string> read_mesh_from_obj(std::istream& obj_stream) {

        std::vector<Point> points;
        std::vector<Normal> normals;
        std::vector<Vec2D> uv_coordinates;
        std::vector<TriangleIndexes> indexes;
        int n_points = 0;

        // Read a valid line from the stream
        auto line_expected = _read_line(obj_stream);
        if (!line_expected.has_value()) {
            return std::unexpected("Problems with the .obj file");
        }

        while (line_expected.has_value()) {

            std::string& line = line_expected.value();
            // Convert the line into an input stream for easier extraction of OBJ data
            std::istringstream ss(line);

            // Reading the line prefix
            // - v: vertex point
            // - vn: vertex normal
            // - vt: vertex texture coordinates
            // - f: polygonal face defined by vertex/UV/normal indices
            std::string prefix;
            ss >> prefix;

            if (prefix == "v") { // Triangles vertices section
                Point v;
                ss >> v.x >> v.y >> v.z;
                points.push_back(v);
            }
            else if (prefix == "vn") { // Triangles normals section
                Normal vn;
                ss >> vn.x >> vn.y >> vn.z;
                vn.normalize();
                normals.push_back(vn);
            }
            else if (prefix == "vt") {
                Vec2D UV;
                ss >> UV.u >> UV.v;
                UV.v = 1.0f - UV.v; // Adapt to the way the code orients test_images axis and UV
                uv_coordinates.push_back(UV);
            }
            else if (prefix == "f") { // Supporting non-triangular mesh files, still storing as triangular mesh

                // Practical data structure for this scope
                struct ObjVertex {
                    int v = 0;   // Position index
                    int vt = 0;  // Texture coordinate index
                    int vn = 0;  // Normal index
                };

                std::vector<ObjVertex> face_vertices;
                std::string vertex_str;

                // Parsing all blocks
                while (ss >> vertex_str) {
                    ObjVertex vertex;

                    // Replacing "/"" with " " for this chunk
                    std::replace(vertex_str.begin(), vertex_str.end(), '/', ' ');
                    // Put the chunk in a string stream
                    std::istringstream vertex_ss(vertex_str);

                    // Extract vertex v/vt/vn
                    if (vertex_ss >> vertex.v >> vertex.vt >> vertex.vn) {
                        face_vertices.push_back(vertex);
                    }
                }

                // Check if it's at least a triangle
                if (face_vertices.size() < 3) {
                    return std::unexpected("Corrupted face format: less than 3 vertex for a face");
                }

                // Index range check:
                // vertex indexes referenced in the f section need to be between 1 and the number of
                // declared vertexes
                for (auto& vertex : face_vertices) {

                    if (vertex.v < 1 || vertex.v > points.size() ||
                        vertex.vn < 1 || vertex.vn > normals.size() ||
                        vertex.vt < 1 || vertex.vt > uv_coordinates.size()) {
                        return std::unexpected("Problems with triangle indexing in .obj file: index out of range");
                    }

                    // Adapting indexing to start from 0
                    vertex.v--;
                    vertex.vt--;
                    vertex.vn--;
                }

                // Transform polygonal faces into triangles: face_vertices contains the vertices of the
                // polygonal face at the line we are reading
                for (int i = 1; i < face_vertices.size() - 1; ++i) {
                    const auto& vA = face_vertices[0]; // Common vertex (fixed pivot)
                    const auto& vB = face_vertices[i];
                    const auto& vC = face_vertices[i + 1];

                    indexes.push_back(TriangleIndexes{
                        vA.v, vB.v, vC.v,
                        vA.vn, vB.vn, vC.vn,
                        vA.vt, vB.vt, vC.vt
                    });
                }
            }

            line_expected = _read_line(obj_stream);
        }

        // Update the data members of the Mesh
        triangle_points_indexes = indexes; // Update the TriangleIndexes data member
        mesh_points = points; // Update the Point data member (mesh points)
        mesh_normals = normals; // Update the Normal data member (mesh points normals)
        mesh_texture_uvs = uv_coordinates; // Update the Vec2D data member (mesh points UV map)

        return {};
    }


    /**
     * @brief Loads a mesh from an OBJ file on disk.
     *
     * Opens the specified file and forwards the parsing work to the stream-based
     * read_mesh_from_obj() overload.
     *
     * @param obj_file Path to the OBJ file.
     * @return An empty expected on success, or an error message if the
     *         file cannot be opened or parsing fails.
     */
    [[nodiscard]] std::expected<void, std::string> read_mesh_from_obj(std::string obj_file) {
        std::ifstream fs(obj_file);
        if (!fs.is_open()) {
            return std::unexpected("Can't find " + obj_file);
        }

        return read_mesh_from_obj(fs);
    }
};
