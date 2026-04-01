export module Geometry;

import std;

export struct Point {
    float x{0.0}, y{0.0}, z{0.0};
};

export struct Vec {

    float x{0.0f}, y{0.0f}, z{0.0f};
};

export struct Norm {
    
    float x{0.0f}, y{0.0f}, z{0.0f};
};


export struct HomMatrix {
    // Homogeneous 4x4 matrix
    std::array<int, 16> mat = {1, 0, 0, 0,
                               0, 1, 0, 0,
                               0, 0, 1, 0,
                               0, 0, 0, 1};
};

template<typename L, typename R, typename Res> Res _sum (L left, R right) {
    return Res{
        left.x + right.x,
        left.y + right.y,
        left.z + right.z
    };
}

template<typename L, typename R, typename Res> Res _difference (L left, R right) {
    return Res{
        left.x - right.x,
        left.y - right.y,
        left.z - right.z
    };
}

template<typename L, typename R, typename Res> Res _negate (L left) {
    return Res{
        -left.x,
        -left.y,
        -left.z
    };
}

template<typename L, typename R, typename Res> Res _dot_product (L left, R right) {
    return Res{
        left.x * right.x,
        left.y * right.y,
        left.z * right.z
    };
}

template<typename L, typename R, typename Res> Res _cross_product (L left, R right) {
    return Res{
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

template<typename L, typename R, typename Res> Res _scalar_multiply (L left, R scalar) {
    return Res{
        left.x * scalar,
        left.y * scalar,
        left.z * scalar
    };
}

template<typename L, typename R, typename Res> Res _scalar_divide (L left, R scalar) {
    return Res{
        left.x / scalar,
        left.y / scalar,
        left.z / scalar
    };
}

template<typename Curr, typename Res> Res _same (Curr left) {
    return Res{
        left.x,
        left.y,
        left.z
    };
}

template<typename Curr, typename Res> Res _norm (Curr left) {
    return std::sqrt(left.x * left.x + left.y * left.y + left.z * left.z);
    
}

export {

    // Sums

    Point operator+= (Point& p, const Vec& v) {
        p = _sum<Point, Vec, Point>(p, v);
        return p;
    }

    Vec operator+= (Vec& v, const Vec& other) {
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

    // Differences

    Point operator-= (Point& p, const Vec& v) {
        p = _difference<Point, Vec, Point>(p, v);
        return p;
    }

    Vec operator-= (Vec& v, const Vec& other) {
        v = _difference<Vec, Vec, Vec>(v, other);
        return v;
    }

    Point operator- (const Point& p, const Vec& v) {
        return _difference<Point, Vec, Point>(p, v);
    }

    Point operator- (const Vec& v, const Point& p) {
        return _difference<Point, Vec, Point>(p, v);
    }

    Vec operator- (const Vec& v, const Vec& other) {
        return _difference<Vec, Vec, Vec>(v, other);
    }

    Vec operator- (const Point& p, const Point& other) {
        return _difference<Point, Point, Vec>(p, other);
    }

    Vec operator- (const Vec& v) {
        return _negate<Vec, void, Vec>(v);
    }

    Point operator- (const Point& p) {
        return _negate<Point, void, Point>(p);
    }

    // Scalar products

    Point operator*= (Point& p, float scalar) {
        return _scalar_multiply<Point, float, Point>(p, scalar);
    }

    Vec operator*= (Vec& v, float scalar) {
        return _scalar_multiply<Vec, float, Vec>(v, scalar);    
    }

    Point operator* (const Point& p, float scalar) {
        return _scalar_multiply<Point, float, Point>(p, scalar);
    }

    Point operator* (float scalar, const Point& p) {
        return _scalar_multiply<Point, float, Point>(p, scalar);
    }

    Vec operator* (const Vec& v, float scalar) {
        return _scalar_multiply<Vec, float, Vec>(v, scalar);
    }

    Vec operator* (float scalar, const Vec& v) {
        return _scalar_multiply<Vec, float, Vec>(v, scalar);
    }

    // Dot products

    float operator* (const Point& p, const Norm& n) {
        auto res = _dot_product<Vec, Norm, Vec>(v, n);
        return res.x + res.y + res.z;
    }

    float operator* (const Vec& v, const Vec& other) {
        auto res = _dot_product<Vec, Vec, Vec>(v, other);
        return res.x + res.y + res.z;
    }

    float operator* (const Vec& v, const Norm& n) {
        auto res = _dot_product<Vec, Norm, Vec>(v, n);
        return res.x + res.y + res.z;
    }

    // Norms

    // RP: please GG, fix the templatization of norm and give here instances. 

    // Conversions

    // RP: Those are methods!
    Vec Point::to_vec() const {
        return _same<Point, Vec>(*this);
    }

    Norm Vec::to_norm() const {
        return *this / this->norm();
    }

};
