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

template<typename L, R, Res> Res _sum (L left, R right) {
    return Res{
        left.x + right.x,
        left.y + right.y,
        left.z + right.z
    };
}

template<typename L, R, Res> Res _difference (L left, R right) {
    return Res{
        left.x - right.x,
        left.y - right.y,
        left.z - right.z
    };
}

template<typename L, R, Res> Res _negate (L left) {
    return Res{
        -left.x,
        -left.y,
        -left.z
    };
}

template<typename L, R, Res> Res _dot_product (L left, R right) {
    return Res{
        left.x * right.x,
        left.y * right.y,
        left.z * right.z
    };
}

template<typename L, R, Res> Res _cross_product (L left, R right) {
    return Res{
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

template<typename L, R, Res> Res _scalar_multiply (L left, R scalar) {
    return Res{
        left.x * scalar,
        left.y * scalar,
        left.z * scalar
    };
}

template<typename L, R, Res> Res _scalar_divide (L left, R scalar) {
    return Res{
        left.x / scalar,
        left.y / scalar,
        left.z / scalar
    };
}

Point operator+= (Point& p, const Vec& v) {
    p = _sum<Point, Vec, Point>(p, v);
    return p;
}

Vec operator*= (Vec& v, const Vec& other) {
    
    v = _sum<Vec, Vec, Vec>(v, other);
    return v;
}

Point operator+ (const Point& p, const Vec& v) {
    return _sum<Point, Vec, Point>(p, v);
}

Point operator+ (const Vec& v, const Point& p) {
    return _sum<Point, Vec, Point>(p, v);
}

Vec operator+ (const Vec& v, const Vec& other) {
    return _sum<Vec, Vec, Vec>(v, other);
}

Point operator-= (Point& p, const Vec& v) {
    p = _difference<Point, Vec, Point>(p, v);
    return p;
}

Point operator- (const Point& p, const Vec& v) {
    return _difference<Point, Vec, Point>(p, v);
}

Point operator- (const Vec& v, const Point& p) {
    return _difference<Point, Vec, Point>(p, v);
}

Vec operator- (const Vec& v) {
    return _negate<Vec, void, Vec>(v);
}

Point operator- (const Point& p) {
    return _negate<Point, void, Point>(p);
}

Vec operator*= (Vec& v, float scalar) {
    return _scalar_multiply<Vec, float, Vec>(v, scalar);    
}
