import std;

struct Point {
    float x{0.0}, y{0.0}, z{0.0};
};

struct Vec {

    float x{0.0f}, y{0.0f}, z{0.0f};
};

struct Norm {
    
    float x{0.0f}, y{0.0f}, z{0.0f};
};

struct HomMatrix {
    // Homogeneous 4x4 matrix
    std::array<int, 16> mat = {1, 0, 0, 0,
                               0, 1, 0, 0,
                               0, 0, 1, 0,
                               0, 0, 0, 1};
};