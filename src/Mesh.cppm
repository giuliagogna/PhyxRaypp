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

// Axis Alligned Bounding Box (AABB) for BVH construction
export struct BVHAABB {
    Point minPoint{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
    Point maxPoint{-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()};

    void grow(const Point& p) {
        minPoint = min(minPoint, p);
        maxPoint = max(maxPoint, p);
    }

    void grow(const BVHAABB& b) {
        minPoint = min(minPoint, b.minPoint);
        maxPoint = max(maxPoint, b.maxPoint);
    }

    // Compute AABB area
    float area() const {
        Vec e = maxPoint - minPoint;
        return 2.0f * (e.x * e.y + e.y * e.z + e.z * e.x);
    }

    // Check which index represent the longest axis
    int longestAxis() const {
        Vec e = maxPoint - minPoint;
        if (e.x > e.y && e.x > e.z) return 0;
        return e.y > e.z ? 1 : 2;
    }

    bool intersect(const Ray& ray) const {
        Vec inv_direction{1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z};

        // Sign logic to avoid if()
        Point bounds[2] = {minPoint, maxPoint};
        int sign[3] = {
            (ray.direction.x < 0.0f),
            (ray.direction.y < 0.0f),
            (ray.direction.z < 0.0f)
        };

        // Math
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

// A point in the mesh cloud, with respective normal
export struct TrianglePoint {
    Point point;
    Normal normal;

    bool is_close(const TrianglePoint& other) {
        return (point.is_close(other.point) && normal.is_close(other.normal));
    }
};

// Indexes to 3 points in the cloud to form a Triangle
export struct TriangleIndexes {
    int i1, i2, i3;

    bool is_equal(const TriangleIndexes& other) {
        return (i1 == other.i1 && i2 == other.i2 && i3 == other.i3);
    }
};

// A bin of the BVH binning method
export struct BVHBin {
    BVHAABB bounds;
    int trianglesCount = 0; // Number of triangles in the bin
};

inline float get_axis_value(const Point& p, int axis) {
    switch (axis) {
        case 0: return p.x;
        case 1: return p.y;
        default: return p.z; // Assumiamo 2 o qualsiasi fallback su Z
    }
}

// Tree node
export struct BVHNode { // RP: seems that alignas() is a secret weapon for performance... I doubt and I will test
    BVHAABB bounds;
    int left_child_index = -1;
    int right_child_index = -1;
    int node_index = -1;

    int minIndex = 0; // This is the first index of the triangle_point_indexes vector that belongs to this node
    int maxIndex = 0; // This is the last index of the triangle_point_indexes

//    std::span<TriangleIndexes> triangle_point_indexes;
    bool is_leaf = false;

    // Pubblic wrapper to call the private method with the default axis value.
    // This is the only safe call so that the tree is correctly built with the best axis at each step.
    // Thus I put this pubblic to avoid calling the dangerous private method.
    void Extend_tree_wrapper(std::vector<BVHNode>& current_nodes, const std::vector<TrianglePoint>& mesh_points, std::vector<TriangleIndexes>& triangle_point_indexes, const int n_bins, const int is_leaf_threshold = 1) {
    
        Extend_tree(current_nodes, current_nodes.size() - 1, mesh_points, triangle_point_indexes, n_bins, is_leaf_threshold);
    }
private:
    // A B S O L U T E     C I N E M A:
    // The this logic breaks since it's possible that the std::vector reallocates. No way.
    // I just declare it static because I don't want any more problems with the std::vector reallocation.
    // I just pass the current index to the recursive calls and I don't care about the actual position of the node in the std::vector.

    // I want this private because it's good to call it with the default axis=3. Otherwise it's possible that it
    // does not explore all the casistics correctly (don't cut on the longest axis at each step) and thus it does not build a good tree.
    static void Extend_tree(std::vector<BVHNode>& current_nodes, int node_index,const std::vector<TrianglePoint>& mesh_points, std::vector<TriangleIndexes>& triangle_point_indexes, const int n_bins, const int is_leaf_threshold = 1, int axis = 3) {
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

            Point centroid = (mesh_points[tri.i1].point +
                              mesh_points[tri.i2].point.to_vec() +
                              mesh_points[tri.i3].point.to_vec()) * (1.0f / 3.0f);

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

        // Binning
        std::vector<BVHBin> bins(n_bins);
        float scale = n_bins / (get_axis_value(centroid_bounds.maxPoint, axis) - get_axis_value(centroid_bounds.minPoint, axis)); // Bins density

        // Select each bin for each triangle based on its centroid
        for (int i = minIndex; i < maxIndex; ++i) {
            Point centroid = (mesh_points[triangle_point_indexes[i].i1].point +
                              mesh_points[triangle_point_indexes[i].i2].point.to_vec() +
                              mesh_points[triangle_point_indexes[i].i3].point.to_vec()) * (1.0f / 3.0f);

            // Find the bin index where the triangle belongs to
            int bin_index = std::max(0, std::min(n_bins - 1, static_cast<int>((get_axis_value(centroid, axis) - get_axis_value(centroid_bounds.minPoint, axis)) * scale)));
            bins[bin_index].trianglesCount++; // Update the population of the bin
            
            // Grow the AABB of the bin
            bins[bin_index].bounds.grow(mesh_points[triangle_point_indexes[i].i1].point);
            bins[bin_index].bounds.grow(mesh_points[triangle_point_indexes[i].i2].point);
            bins[bin_index].bounds.grow(mesh_points[triangle_point_indexes[i].i3].point);
        }

        // SAH formula (to be minimized):
        // C ~ AreaL * CountL + AreaR * CountR (simplified formula neglecting tree branching overhead)    

        // We compute the left term values
        std::vector<float> left_bounds_area(n_bins - 1);
        std::vector<int> left_count(n_bins - 1);
        int left_sum = 0; 
        BVHAABB left_box;
        for (int i = 0; i < n_bins - 1; ++i) {
            left_sum += bins[i].trianglesCount; // Cumulative sum from the left
            left_box.grow(bins[i].bounds); // Grow the AABB from the left
            left_bounds_area[i] = left_box.area(); // Compute the area at this point
            left_count[i] = left_sum; // Count the triangles 
        }

        // Same stuff but from the right. Of course indexes run backwards
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
        float scale_plane = (get_axis_value(centroid_bounds.maxPoint, axis) - get_axis_value(centroid_bounds.minPoint, axis)) / n_bins;

        for (int i = 0; i < n_bins - 1; ++i) {
            float cost = left_bounds_area[i] * left_count[i] + right_bounds_area[i] * right_count[i];
            if (cost < best_cost) {
                best_cost = cost;
                split_value = get_axis_value(centroid_bounds.minPoint, axis) + (i + 1) * scale_plane;
            }
        }

        // No split cost
        float no_split_cost = current_nodes[node_index].bounds.area() * (maxIndex - minIndex);
        if (best_cost >= no_split_cost) {
            current_nodes[node_index].is_leaf = true;
            return;
        }

        // Now we have the split_value to cut the current AABB!

        // Reordering the std::vector segment: 
        // I reorder the std::vector segment I see from the window to have the
        // left child members at the start of the window and the right child ones at the end
        int left_maxIndex = minIndex; // This will be the max index of the left child, so it will be the first index of the right child
        {
            int i = minIndex;
            int j = maxIndex - 1;

            while (i <= j) {
                Point centroid = (mesh_points[triangle_point_indexes[i].i1].point +
                                  mesh_points[triangle_point_indexes[i].i2].point.to_vec() +
                                  mesh_points[triangle_point_indexes[i].i3].point.to_vec()) * (1.0f / 3.0f);

                if (get_axis_value(centroid, axis) < split_value) {
                    i++;
                } else {
                    std::swap(triangle_point_indexes[i], triangle_point_indexes[j]);
                    j--; // I don't swap it anymore
                }
            }        
            left_maxIndex = i;
        }

        // Check if there is a better cutting for the AABB along other axis
        if (left_maxIndex == minIndex || left_maxIndex == maxIndex) {
            axis = (axis + 1) % 3; // Cicle on axis indexes
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

        // Update datamembers of the children
        // Triangle points indexes of the child 
        current_nodes[current_nodes[node_index].left_child_index].minIndex = minIndex;
        current_nodes[current_nodes[node_index].left_child_index].maxIndex = left_maxIndex;
        current_nodes[current_nodes[node_index].right_child_index].minIndex = left_maxIndex;
        current_nodes[current_nodes[node_index].right_child_index].maxIndex = maxIndex;

        // Update the AABB of the children
        current_nodes[current_nodes[node_index].left_child_index].bounds = BVHAABB();
        for (int i = current_nodes[current_nodes[node_index].left_child_index].minIndex; i < current_nodes[current_nodes[node_index].left_child_index].maxIndex; ++i) {
            current_nodes[current_nodes[node_index].left_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].i1].point);
            current_nodes[current_nodes[node_index].left_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].i2].point);
            current_nodes[current_nodes[node_index].left_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].i3].point);
        }
        current_nodes[current_nodes[node_index].right_child_index].bounds = BVHAABB();
        for (int i = current_nodes[current_nodes[node_index].right_child_index].minIndex; i < current_nodes[current_nodes[node_index].right_child_index].maxIndex; ++i) {
            current_nodes[current_nodes[node_index].right_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].i1].point);
            current_nodes[current_nodes[node_index].right_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].i2].point);
            current_nodes[current_nodes[node_index].right_child_index].bounds.grow(mesh_points[triangle_point_indexes[i].i3].point);
        }

        // Call this method to generate the entire tree recursively
        current_nodes[current_nodes[node_index].left_child_index].Extend_tree(current_nodes, current_nodes[node_index].left_child_index, mesh_points, triangle_point_indexes, n_bins, is_leaf_threshold);
        current_nodes[current_nodes[node_index].right_child_index].Extend_tree(current_nodes, current_nodes[node_index].right_child_index, mesh_points, triangle_point_indexes, n_bins, is_leaf_threshold);

        return;
    }    
};

// ===============
// BVH Mesh
// ===============

export struct Mesh : Shape {

    std::vector<TrianglePoint> mesh_points;
    std::vector<TriangleIndexes> triangle_points_indexes;
    std::vector<BVHNode> nodes;

    using Shape::Shape;

    Mesh(
        const Transformation& trans,
        std::shared_ptr<Material> material,
        std::vector<TrianglePoint> points,
        std::vector<TriangleIndexes> indexes,
        std::vector<BVHNode> bvh_nodes
    ) : Shape(trans, material), 
        mesh_points(std::move(points)),
        triangle_points_indexes(std::move(indexes)),
        nodes(std::move(bvh_nodes)) {

    }
    // Transformation not supported yet!!!
    Mesh(std::string obj_file, std::shared_ptr<Material> material = nullptr, int BVH_n_bins = 12, int BVH_is_leaf_threshold = 3) : Shape(Transformation{}, material) {
        // Call read_mesh_from_obj(obj_file)
        auto file_reading_result = read_mesh_from_obj(obj_file);
        if (!file_reading_result.has_value()) {
            std::println("ERROR: ");
            std::print("{}", file_reading_result.error());
            std::print("\n --- from constructor Mesh::Mesh(std::string, std::shared_ptr<Material>)");
            return;
        }
        // Work on the creation of the root node
        BVHNode root;
        // Put minIndex and maxIndex
        root.minIndex = 0;
        root.maxIndex = triangle_points_indexes.size();
        // Extend AABB of the root
        for (auto& mesh_point : mesh_points) {
            root.bounds.grow(mesh_point.point);
        }
        nodes.push_back(root);
        // Call BVHNode::Extend_tree via wrapper
        nodes[0].Extend_tree_wrapper(nodes, mesh_points, triangle_points_indexes, BVH_n_bins, BVH_is_leaf_threshold);
    }

    [[nodiscard]] std::optional<HitRecord> ray_intersection(const Ray& ray) const override {
        // Reference frame of the mesh
        Ray local_ray = ray.transform(trans.inverse());
        auto hit_record = ray_intersection_unwrapped(local_ray);
        if (hit_record.has_value()) {
            hit_record->ray = ray;
            hit_record->hit_point = ray.at(hit_record->t);
            hit_record->hit_normal = (trans * hit_record->hit_normal).normalize();
        }
        return hit_record;

    }

    [[nodiscard]] std::optional<HitRecord> ray_intersection_unwrapped(Ray& local_ray, int starting_index = 0, std::optional<HitRecord> closest_hit = std::nullopt) const {
        if (!nodes[starting_index].bounds.intersect(local_ray)) {
            return closest_hit; // No intersection with the bounding box, skip this node
        }

        if (nodes[starting_index].is_leaf) {
            // Check intersection with triangles in this leaf node
            for (int i = nodes[starting_index].minIndex; i < nodes[starting_index].maxIndex; ++i) {
                const auto& tri = triangle_points_indexes[i];
                const auto& A = mesh_points[tri.i1];
                const auto& B = mesh_points[tri.i2];
                const auto& C = mesh_points[tri.i3];

                // Ray-triangle intersection test (Möller–Trumbore algorithm)
                Vec E1 = B.point - A.point;
                Vec E2 = C.point - A.point;
                Vec P = local_ray.direction % E2;
                Vec T = local_ray.origin - A.point;
                Vec Q = T % E1;
                float det = E1 * P;
                // Early exit if ray is parallel or nearly parallel to the triangle plane
                if (std::abs(det) < 1e-6f) {
                    continue; 
                }
                float inv_det = 1.0f / det;
                float u = (T * P) * inv_det;
                float v = (Q * local_ray.direction) * inv_det;
                float t = (Q * E2) * inv_det;
                if (u >= 0 && v >= 0 && u + v <= 1 && t >= local_ray.tmin && t <= local_ray.tmax) {
                    // Some members of the HitRecord struct will be updated outside this method,
                    // since we are in the frame of reference of the Mesh.
                    HitRecord hit;
                    hit.t = t;
                    Vec interpolated_normal = (A.normal * (1 - u - v) + B.normal * u + C.normal * v);
                    hit.hit_normal = (interpolated_normal * (-std::copysign(1.0f, interpolated_normal * local_ray.direction))).to_norm(); // Flip normal if it's facing the ray
                    hit.surface_params = {u, v}; // Will be updated in the future with actual UV coordinates if available
                    hit.hitted_shape = this; // Point directly to this exact mesh in memory
                    // Uodate local ray tmax
                    local_ray.tmax = t; // Shrink the ray range to find closer intersections
                    closest_hit = hit; // Update the closest hit
                }
            }
            return closest_hit;

        } else {
            // Recursion on children
            // 3. INTERNAL NODE: Recursive traversal
            if (auto left_hit = ray_intersection_unwrapped(local_ray, nodes[starting_index].left_child_index, closest_hit)) {
                local_ray.tmax = left_hit->t; // Shrink the ray range to find closer intersections
                closest_hit = left_hit;
            }

            if (auto right_hit = ray_intersection_unwrapped(local_ray, nodes[starting_index].right_child_index, closest_hit)) {
                local_ray.tmax = right_hit->t; // Shrink the ray range to find closer intersections
                closest_hit = right_hit; // Guaranteed to be closer if found because local_ray.tmax was shrunk
            }
            return closest_hit; // Return the closest hit found in the children
        }
    }
    // Reads a line in the header, skipping comments (lines starting with #) and empty lines.
    [[nodiscard]] std::expected<std::string, std::string> _read_line(std::istream& stream) {
        std::string result;
        
        // read till we find a line that is not a comment (starting with #) and not empty (for example an extra newline)
        while (std::getline(stream, result)) {
            // remove carriage return \r
            if (!result.empty() && result.ends_with("\r")) {
                result.pop_back();
            }

            // if the line starts with # it is a comment and we skip it (flexibility)
            if (result.empty() || result.starts_with("#") || result.starts_with("o")) {
                continue;
            }

            return result;
        }

        return std::unexpected("Impossible to read line.");
    }

    [[nodiscard]] std::expected<void, std::string> read_mesh_from_obj(std::istream& obj_stream) {
        std::vector<Point> just_points;
        std::vector<Normal> just_normals;
        std::vector<TriangleIndexes> indexes;
        std::vector<TrianglePoint> points_and_normals;
        int n_points = 0;
        bool has_entered_while_cycle = false;

        // Read a valid line from the stream
        auto line_expected = _read_line(obj_stream);
        while (line_expected.has_value()) {
            has_entered_while_cycle = true;
            // Clean from the / / / (if we are in the faces field of the file)
            std::string& line = line_expected.value();
            std::replace(line.begin(), line.end(), '/', ' ');
            // Reput in a stream
            std::istringstream ss(line); // Using a string stream to parse things
            // Reading the line prefix
            // v: vertex point
            // vn: vertex normal
            // vt: vertex texture coordinates (not supported yet)
            // f: triangular face points and normals indexes
            std::string prefix;
            ss >> prefix;

            if (prefix == "v") { // Triangles verticies section
                Point v;
                ss >> v.x >> v.y >> v.z; 
                just_points.push_back(v);
            } 
            else if (prefix == "vn") { // Triangles normals section
                Normal vn;
                ss >> vn.x >> vn.y >> vn.z;
                vn.normalize();
                just_normals.push_back(vn);
            } 
            else if (prefix == "f") { // Triangles section
                if (!n_points) {
                    n_points = just_normals.size();
                    if(n_points != just_points.size()) {
                        return std::unexpected("Points and Normals std::vector are different in size, please check the .obj file");
                    }                    
                }

                // Reading the indexes
                int v1, vn1, v2, vn2, v3, vn3, junk;
                ss >> v1 >> junk >> vn1
                   >> v2 >> junk >> vn2
                   >> v3 >> junk >> vn3;
                v1--; vn1--; v2--; vn2--; v3--; vn3--; // .obj files indexing starts from 1 so they are readapted to start drom 0 to match this code
                
                if (v1 != vn1 || v2 != vn2 || v3 != vn3) {
                    return std::unexpected("Problems with triangle indexing in .obj file: for a single triangle Point and Normal should be under the same index");
                }
                if (v1 >= n_points || v2 >= n_points || v3 >= n_points) {
                    return std::unexpected("Problems with triangle indexing in .obj file: index out of range");
                }

                indexes.push_back(TriangleIndexes{v1, v2, v3});
            }

            // Avanziamo alla riga successiva prima di ricominciare il ciclo
            line_expected = _read_line(obj_stream);
        }
        if (!has_entered_while_cycle) {
            return std::unexpected("Problems with the .obj file");
        }
        triangle_points_indexes = indexes; // Update the object data member

        for (int i = 0; i < n_points; i++) {
            points_and_normals.push_back({just_points[i], just_normals[i]});
        }
        mesh_points = points_and_normals;
        return {};       
    }

    // wrapper: you pass the file name string to call read_mesh_from_obj
    [[nodiscard]] std::expected<void, std::string> read_mesh_from_obj(std::string obj_file) {
        std::ifstream fs(obj_file);
        if (!fs.is_open()) {
            return std::unexpected("Can't find " + obj_file);
        }

        return read_mesh_from_obj(fs);
    }
};
