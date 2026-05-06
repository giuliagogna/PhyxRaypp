module;

export module AABB;
import std;
import Geometry;
import Camera;
import auxiliary_functions;
import Color;
import HDRImage;
import Shape;

// ================================================================
// AABB STRUCTURE
// ===============================================================

//export struct AABB {
//
//    
//
//    std::vector<std::shared_ptr<Shape>> shapes; // Shapes intersecting the AABB
//
//    void is_inside(std::shared_ptr<Shape> shape); // Check if a shape is inside the AABB and add it to the list of shapes if it is
//
//    std::pair<AABB, AABB> fork() const; // Split the AABB into two child AABBs selecting the best bisection)
//    [[nodiscard]] bool ray_intersection(const Ray& ray) const; // Check if a ray intersects the AABB
//}

//std::pair<AABB, AABB> AABB::fork() const {
//    // Compute the center of the AABB
//    float x_center = (x_min + x_max) / 2.0f;
//    float y_center = (y_min + y_max) / 2.0f;
//    float z_center = (z_min + z_max) / 2.0f;
//
//    // Compute the extents of the AABB
//    float x_extent = x_max - x_min;
//    float y_extent = y_max - y_min;
//    float z_extent = z_max - z_min;
//
//    AABB left_aabb, right_aabb; // Initialize the child AABBs with the same bounds as the parent
//
//    // case 1: split along x-axis
//    left_aabb.x_min = x_min;
//    left_aabb.x_max = x_center;
//    left_aabb.y_min = y_min;
//    left_aabb.y_max = y_max;
//    left_aabb.z_min = z_min;
//    left_aabb.z_max = z_max;
//
//    right_aabb.x_min = x_center;
//    right_aabb.x_max = x_max;
//    right_aabb.y_min = y_min;
//    right_aabb.y_max = y_max;
//    right_aabb.z_min = z_min;
//    right_aabb.z_max = z_max;
//
//    std::vector<std::shared_ptr<Shape>> left_shapes, right_shapes; // Vectors to hold the shapes for each child AABB
//    for (const auto& shape : shapes) {
//        float shape_x_min, shape_x_max, shape_y_min, shape_y_max, shape_z_min, shape_z_max; // Get the bounding box of the shape
//
//        
//    }
//
//
//
//
//
//}

// ================================================================
// AABB NODE STRUCTURE
// ================================================================

// std::shared_ptr based binary graph for BVH
export struct BVH_Node {

    AABB aabb; // The AABB for this node
    std::shared_ptr<BVH_Node> left; // Pointer to the left child node
    std::shared_ptr<BVH_Node> right; // Pointer to the right child node
    bool is_leaf = false;
    int leaf_limit = 2;
    std::vector<std::shared_ptr<Shape>> shapes;

    void BVHNode::build(std::vector<std::shared_ptr<Shape>> current_shapes, int leaf_limit) {
    // 1. Calcola la AABB totale per questo nodo
    for (const auto& s : current_shapes) {
        this->aabb.expand_to_include(s->get_AABB());
    }

    // 2. CONDIZIONE DI ARRESTO: se siamo sotto il limite, è una foglia
    if (current_shapes.size() <= leaf_limit) {
        this->shapes = std::move(current_shapes);
        this->left = nullptr;
        this->right = nullptr;
        return;
    }

//    // 3. SCEGLI L'ASSE PIÙ LUNGO (per bisezioni più quadrate)
//    int axis = this->box.longest_axis(); // 0 per X, 1 per Y, 2 per Z

    // 4. BISEZIONE SPAZIALE
    float split_pos = this->box.center(axis);
    std::vector<std::shared_ptr<Shape>> left_list;  // The choosen one
    std::vector<std::shared_ptr<Shape>> right_list; // The choosen one

    // We need to try which split is more performative, best splitting the data

    std::vector<std::shared_ptr<Shape>> x_left_list;  // The x candidate
    std::vector<std::shared_ptr<Shape>> x_right_list; // The x candidate
    std::vector<std::shared_ptr<Shape>> y_left_list;  // The y candidate
    std::vector<std::shared_ptr<Shape>> y_right_list; // The y candidate
    std::vector<std::shared_ptr<Shape>> z_left_list;  // The z candidate
    std::vector<std::shared_ptr<Shape>> z_right_list; // The z candidate

    for (auto& s : current_shapes) {
        // Usiamo il centroide dell'oggetto per decidere in quale ramo mandarlo.
        // Questo garantisce che ogni oggetto vada in UN SOLO figlio, 
        // evitando duplicazioni pesanti nell'albero.
        if (s->get_AABB().center(axis) < split_pos) {
            left_list.push_back(std::move(s));
        } else {
            right_list.push_back(std::move(s));
        }
    }

    // 5. GESTIONE DI EMERGENZA: se un lato è vuoto, forza una divisione a metà del vettore
    // (Succede se molti oggetti hanno lo stesso centroide)
    if (left_list.empty() || right_list.empty()) {
        left_list.clear();
        right_list.clear();
        size_t mid = current_shapes.size() / 2;
        for (size_t i = 0; i < current_shapes.size(); ++i) {
            if (i < mid) left_list.push_back(current_shapes[i]);
            else right_list.push_back(current_shapes[i]);
        }
    }

    // 6. RICORSIONE
    this->left = std::make_shared<BVHNode>();
    this->left->build(std::move(left_list), leaf_limit);

    this->right = std::make_shared<BVHNode>();
    this->right->build(std::move(right_list), leaf_limit);
}


};