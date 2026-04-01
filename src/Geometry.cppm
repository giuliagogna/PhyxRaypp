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

template<typename L, R, Res> Res _sum (L left, R right) {
    return Out{
        left.x + right.x,
        left.y + right.y,
        left.z + right.z
    };
}

template<typename L, R, Res> Res _difference (L left, R right) {
    return Out{
        left.x - right.x,
        left.y - right.y,
        left.z - right.z
    };
}

template<typename L, R, Res> Res _negate (L left) {
    return Out{
        -left.x,
        -left.y,
        -left.z
    };
}

template<typename L, R, Res> Res _dot_product (L left, R right) {
    return Out{
        left.x * right.x,
        left.y * right.y,
        left.z * right.z
    };
}

template<typename L, R, Res> Res _cross_product (L left, R right) {
    return Out{
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

template<typename L, R, Res> Res _scalar_multiply (L left, R scalar) {
    return Out{
        left.x * scalar,
        left.y * scalar,
        left.z * scalar
    };
}

template<typename L, R, Res> Res _scalar_divide (L left, R scalar) {
    return Out{
        left.x / scalar,
        left.y / scalar,
        left.z / scalar
    };
}

struct HomMatrix {
    // Homogeneous 4x4 matrix
    std::array<int, 16> mat = {1, 0, 0, 0,
                               0, 1, 0, 0,
                               0, 0, 1, 0,
                               0, 0, 0, 1};
};
