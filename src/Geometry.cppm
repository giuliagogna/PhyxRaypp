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

//#include <format>

export module Geometry;

import auxiliary_functions;
import std;

// Normalized 3D vector, used for normals (and directions???)
export struct Normal {
    float x{0.0f}, y{0.0f}, z{0.0f};

    float norm() const;  // Compute length of Normal object
    float norm2() const; // Compute length square of Normal object

    Normal normalize(); // normalizes the Normal object (non-const)

    bool is_close(const Normal& other, float epsilon = 1e-5f) const; // Check if two Normals are close enough
};

// 3D vector, used for distances (and directions???)
export struct Vec {
    float x{0.0f}, y{0.0f}, z{0.0f};

    Normal to_norm() const; // 
    Vec normalize() const; // normalizes and returns a Vec

    // Compute length and length square
    float norm() const;
    float norm2() const;

    // GG: Check if two vectors are close enough: written as a method as suggested in the lecture slides
    bool is_close(const Vec& other, float epsilon = 1e-5f) const;
};

// 3D point, used for positions
export struct Point {
    float x{0.0}, y{0.0}, z{0.0};
    Vec to_vec() const;
    Normal to_norm() const;
    float norm() const;
    float norm2() const;
    void normalize(); // normalizes the Point object (non-const)

    // GG: Check if two Points are close enough: written as a method as suggested in the lecture slides
    bool is_close(const Point& other, float epsilon = 1e-5f) const;
};

// Data structure to store a 4x4 matricial like dataset, used for homogeneous transformations
export struct HomMatrix {
    // GG: This is only the basic object that stores a 4x4 Homogeneous Matrix (inverse matrix
    // and consistency checks will be implemented inside Transformation struct)
    std::array<float, 16> mat = {1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f};

    // GG: checks if two matrixes are equal
    bool is_close(const HomMatrix& other, float epsilon = 1e-5f) const;
};

export struct Transformation {
    HomMatrix m;
    HomMatrix invm;

    bool is_consistent() const;

    // Method to apply the inverse transformation
    // GG: Note that when we apply transformation to a Point or Vec we use the direct matrix.
    //     As pointed out in lecture, we often want to apply the inverse transformation: to do so
    //     we implement a method that simply switches the two matrixes, so that now the inverse is
    //     the direct and vice-versa
    Transformation inverse() const;
};


// ================================================
// ALGEBRA TEMPLATE FUNCTIONS
// ================================================

template<typename L, typename R, typename Res> Res _sum (const L& left, const R& right) {
    return Res{
        left.x + right.x,
        left.y + right.y,
        left.z + right.z
    };
}

template<typename L, typename R, typename Res> Res _difference (const L& left, const R& right) {
    return Res{
        left.x - right.x,
        left.y - right.y,
        left.z - right.z
    };
}

template<typename L, typename R, typename Res> Res _negate (const L& left) {
    return Res{
        -left.x,
        -left.y,
        -left.z
    };
}

template<typename L, typename R, typename Res> Res _elementwise_product (const L& left, const R& right) {
    return Res{
        left.x * right.x,
        left.y * right.y,
        left.z * right.z
    };
}

template<typename L, typename R, typename Res> Res _cross_product (const L& left, const R& right) {
    return Res{
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

template<typename L, typename R, typename Res> Res _scalar_multiply (const L& left, const R& scalar) {
    return Res{
        left.x * scalar,
        left.y * scalar,
        left.z * scalar
    };
}

template<typename L, typename R, typename Res> Res _scalar_divide (const L& left, const R& scalar) {
    return Res{
        left.x / scalar,
        left.y / scalar,
        left.z / scalar
    };
}

template<typename Curr, typename Res> Res _same (const Curr& left) {
    return Res{
        left.x,
        left.y,
        left.z
    };
}


// ================================================
// OPERATORS OVERLOAD
// ================================================

export {

    // ================================================
    // OPERATIONS ON FUNDAMENTAL OBJECTS
    // ================================================

    // Sums
    /// Point += Vec -> Point
    Point& operator+= (Point& p, const Vec& v) {
        p = _sum<Point, Vec, Point>(p, v);
        return p;
    }
    /// Point + Vec -> Point
    Point operator+ (const Point& p, const Vec& v) {
        return _sum<Point, Vec, Point>(p, v);
    }

    // GG: Technically given the sum is commutative one could decide to sum Vec + Point, but I would
    // avoid it for logical coherence

    /// Vec += Vec -> Vec
    Vec& operator+= (Vec& v, const Vec& other) {
        v = _sum<Vec, Vec, Vec>(v, other);
        return v;
    }

    /// Vec + Vec -> Vec
    Vec operator+ (const Vec& v, const Vec& other) {
        return _sum<Vec, Vec, Vec>(v, other);
    }

    // Differences

    /// Point-=Vec -> Point
    Point& operator-= (Point& p, const Vec& v) {
        p = _difference<Point, Vec, Point>(p, v);
        return p;
    }
    /// Point - Vec -> Point
    Point operator- (const Point& p, const Vec& v) {
        return _difference<Point, Vec, Point>(p, v);
    }

    /// Vec -= Vec -> Vec
    Vec& operator-= (Vec& v, const Vec& other) {
        v = _difference<Vec, Vec, Vec>(v, other);
        return v;
    }

    /// Vec - Vec -> Vec
    Vec operator- (const Vec& v, const Vec& other) {
        return _difference<Vec, Vec, Vec>(v, other);
    }

    /// Point - Point -> Vec
    Vec operator- (const Point& p, const Point& other) {
        return _difference<Point, Point, Vec>(p, other);
    }


    // Negations
    /// -Vec -> Vec
    Vec operator- (const Vec& v) {
        return _negate<Vec, void, Vec>(v);
    }

    /// -Point -> Point
    Point operator- (const Point& p) {
        return _negate<Point, void, Point>(p);
    }

    /// -Normal -> Normal
    Normal operator- (const Normal& n) {
        return _negate<Normal, void, Normal>(n);
    }
    // Scalar products

    // GG: Why commented?
    // RP: prof didn't wanted in the lectures

//    /// Point * scalar -> Point
//    Point operator*= (Point& p, float scalar) {
//        // GG: Need to assign the result to the calling object
//        p = _scalar_multiply<Point, float, Point>(p, scalar);
//        return p;
//    }
//
//    Point operator* (const Point& p, float scalar) {
//        return _scalar_multiply<Point, float, Point>(p, scalar);
//    }
//
//    Point operator* (float scalar, const Point& p) {
//        return _scalar_multiply<Point, float, Point>(p, scalar);
//    }

    /// Vec*=scalar -> Vec
    Vec& operator*= (Vec& v, float scalar) {
        // GG: Need to assign the result to the calling object
        v = _scalar_multiply<Vec, float, Vec>(v, scalar);
        return v;
    }
    /// Vec * scalar -> Vec
    Vec operator* (const Vec& v, float scalar) {
        return _scalar_multiply<Vec, float, Vec>(v, scalar);
    }
    /// Scalar * Vec -> Vec
    Vec operator* (float scalar, const Vec& v) {
        return _scalar_multiply<Vec, float, Vec>(v, scalar);
    }

    ///Normal * scalar -> Vec
    Vec operator* (const Normal& n, float scalar) {
        return _scalar_multiply<Normal, float, Vec>(n, scalar);
    }
    /// Scalar * Normal -> Vec
    Vec operator* (float scalar, const Normal& n) {
        return _scalar_multiply<Normal, float, Vec>(n, scalar);
    }

    // Scalar division

    /// Scalar division between a Vec and a scalar
    Vec& operator/= (Vec& v, float scalar) {
        v = _scalar_divide<Vec, float, Vec>(v, scalar);
        return v;
    }

    /// Scalar division between a Vec and a scalar
    Vec operator/ (const Vec& v, float scalar) {
        return _scalar_divide<Vec, float, Vec>(v, scalar);
    }

    // GG: Note that if you divide a Normal for a scalar it is not a normal anymore
    //     becomes a vector
    /// Scalar division between a Normal and a scalar
    //Normal& operator/= (Normal& n, float scalar) {
    //    n = _scalar_divide<Normal, float, Normal>(n, scalar);
    //    return n;
    //}
    /// Scalar division between a Normal and a scalar
    Vec operator/ (const Normal& n, float scalar) {
        return _scalar_divide<Normal, float, Vec>(n, scalar);
    }

    // needed for homogeneous division in Point operator* (const HomMatrix& M, const Point& p)
    /// Scalar division between a Point and a scalar
    Point operator/ (const Point& p, float scalar) {
        return _scalar_divide<Point, float, Point>(p, scalar);
    }

    // Dot products

    /// Dot product between two Vec
    float operator* (const Vec& v, const Vec& other) {
        auto res = _elementwise_product<Vec, Vec, Vec>(v, other);
        return res.x + res.y + res.z;
    }
    /// Dot products between a Vec and a Normal
    float operator* (const Vec& v, const Normal& n) {
        auto res = _elementwise_product<Vec, Normal, Vec>(v, n);
        return res.x + res.y + res.z;
    }
    ///Dot products between a Normal and a Vec
    float operator* (const Normal& n, const Vec& v) {
        auto res = _elementwise_product<Normal, Vec, Vec>(n, v);
        return res.x + res.y + res.z;
    }
    /// Dot product between two Normal
    float operator* (const Normal& v, const Normal& n) {
        auto res = _elementwise_product<Normal, Normal, Vec>(v, n);
        return res.x + res.y + res.z;
    }

    // Cross products

    /// Cross product between two Vec
    Vec operator% (const Vec& v, const Vec& other) {
        return _cross_product<Vec, Vec, Vec>(v, other);
    }

    /// Cross product between a Vec and a Normal  
    Vec operator% (const Vec& v, const Normal& n) {
        return _cross_product<Vec, Normal, Vec>(v, n);
    }

    /// Cross product between a Normal and a Vec
    Vec operator% (const Normal& n, const Vec& v) {
        return _cross_product<Normal, Vec, Vec>(n, v);
    }

    /// Cross product between two Normal
    Vec operator% (const Normal& n, const Normal& other) {
        return _cross_product<Normal, Normal, Vec>(n, other);
    }

    // Matrix multiplication

    // Homogeneous matrix multiplication with Point, Vec and Normal (returns a Point, Vec or Normal with the same type of the first argument)

    Point operator* (const HomMatrix& M, const Point& p) {

        float px = p.x; float py = p.y; float pz = p.z; // This should be optimized by the compiler to avoid overhead of multiple accesses to p.x, p.y and p.z

        Point res{
            M.mat[0] * px + M.mat[1] * py + M.mat[2] * pz + M.mat[3],
            M.mat[4] * px + M.mat[5] * py + M.mat[6] * pz + M.mat[7],
            M.mat[8] * px + M.mat[9] * py + M.mat[10] * pz + M.mat[11]
        };

        float w = M.mat[12] * px + M.mat[13] * py + M.mat[14] * pz + M.mat[15];

        // RP: Is w==1.f likely to happen after all the roundings and stuff? I'll leave this commented then we will see.
        // GG: yes it is possible since the basic transformations we are going to use are affine transformations (which
        // last row is going to be exactly 0.0 0.0 0.0 1.0)
        if (w==1.f) {
            return res;
        }
        
        return res / w; // homogeneous division
    }

    Vec operator* (const HomMatrix& M, const Vec& v) {
        return Vec{
            M.mat[0] * v.x + M.mat[1] * v.y + M.mat[2] * v.z,
            M.mat[4] * v.x + M.mat[5] * v.y + M.mat[6] * v.z,
            M.mat[8] * v.x + M.mat[9] * v.y + M.mat[10] * v.z
        };
    }


    HomMatrix operator* (const HomMatrix& M1, const HomMatrix& M2) {
        
        return HomMatrix{
            M1.mat[0] * M2.mat[0] + M1.mat[1] * M2.mat[4] + M1.mat[2] * M2.mat[8] + M1.mat[3] * M2.mat[12],
            M1.mat[0] * M2.mat[1] + M1.mat[1] * M2.mat[5] + M1.mat[2] * M2.mat[9] + M1.mat[3] * M2.mat[13],
            M1.mat[0] * M2.mat[2] + M1.mat[1] * M2.mat[6] + M1.mat[2] * M2.mat[10] + M1.mat[3] * M2.mat[14],
            M1.mat[0] * M2.mat[3] + M1.mat[1] * M2.mat[7] + M1.mat[2] * M2.mat[11] + M1.mat[3] * M2.mat[15],

            M1.mat[4] * M2.mat[0] + M1.mat[5] * M2.mat[4] + M1.mat[6] * M2.mat[8] + M1.mat[7] * M2.mat[12],
            M1.mat[4] * M2.mat[1] + M1.mat[5] * M2.mat[5] + M1.mat[6] * M2.mat[9] + M1.mat[7] * M2.mat[13],
            M1.mat[4] * M2.mat[2] + M1.mat[5] * M2.mat[6] + M1.mat[6] * M2.mat[10] + M1.mat[7] * M2.mat[14],
            M1.mat[4] * M2.mat[3] + M1.mat[5] * M2.mat[7] + M1.mat[6] * M2.mat[11] + M1.mat[7] * M2.mat[15],

            M1.mat[8] * M2.mat[0] + M1.mat[9] * M2.mat[4] + M1.mat[10] * M2.mat[8] + M1.mat[11] * M2.mat[12],
            M1.mat[8] * M2.mat[1] + M1.mat[9] * M2.mat[5] + M1.mat[10] * M2.mat[9] + M1.mat[11] * M2.mat[13],
            M1.mat[8] * M2.mat[2] + M1.mat[9] * M2.mat[6] + M1.mat[10] * M2.mat[10] + M1.mat[11] * M2.mat[14],
            M1.mat[8] * M2.mat[3] + M1.mat[9] * M2.mat[7] + M1.mat[10] * M2.mat[11] + M1.mat[11] * M2.mat[15],

            M1.mat[12] * M2.mat[0] + M1.mat[13] * M2.mat[4] + M1.mat[14] * M2.mat[8] + M1.mat[15] * M2.mat[12],
            M1.mat[12] * M2.mat[1] + M1.mat[13] * M2.mat[5] + M1.mat[14] * M2.mat[9] + M1.mat[15] * M2.mat[13],
            M1.mat[12] * M2.mat[2] + M1.mat[13] * M2.mat[6] + M1.mat[14] * M2.mat[10] + M1.mat[15] * M2.mat[14],
            M1.mat[12] * M2.mat[3] + M1.mat[13] * M2.mat[7] + M1.mat[14] * M2.mat[11] + M1.mat[15] * M2.mat[15]
        };
    }


    // ================================================
    // OPERATIONS ON TRANSFORMATIONS
    // ================================================

    /// Transformation composition
    Transformation operator*(const Transformation& T1, const Transformation& T2) {
        return Transformation{
            T1.m * T2.m,            // Direct transformation multiplies in order
            T2.invm * T1.invm   // Inverse transformation multiplies switched
        };
    }

    /// Transformation of a Point
    Point operator*(const Transformation& T, const Point& p) {
        return T.m * p;
    }

    /// Transformation of a Vec
    Vec operator*(const Transformation& T, const Vec& v) {
        return T.m * v;
    }

    /// Transformation of a Normal
    Normal operator* (const Transformation& T, const Normal& n) {
        return Normal{
            T.invm.mat[0] * n.x + T.invm.mat[4] * n.y + T.invm.mat[8] * n.z,
            T.invm.mat[1] * n.x + T.invm.mat[5] * n.y + T.invm.mat[9] * n.z,
            T.invm.mat[2] * n.x + T.invm.mat[6] * n.y + T.invm.mat[10] * n.z
        };
    }



    // ================================================
    // TRANSFORMATION GENERATORS
    // ================================================

    /// Generates a translation Transformation
    Transformation Tras(const Vec& v) {
        Transformation t; // Starts as Identity
        // M
        // Sets the last column to the components of the vector
        t.m.mat[3] = v.x;
        t.m.mat[7] = v.y;
        t.m.mat[11] = v.z;
        // Inverse is just a translation by -v
        t.invm.mat[3] = -v.x;
        t.invm.mat[7] = -v.y;
        t.invm.mat[11] = -v.z;
        return t;
    }

    /// Generates a scaling Transformation
    Transformation Scale(const Vec& v) {
        Transformation t; // Starts as Identity
        // M
        // Sets diagonal elements to components of the scaling vector
        t.m.mat[0] = v.x;
        t.m.mat[5] = v.y;
        t.m.mat[10] = v.z;
        // Inverse is just a scaling by 1/v
        t.invm.mat[0] = 1.0f / v.x;
        t.invm.mat[5] = 1.0f / v.y;
        t.invm.mat[10] = 1.0f / v.z;
        return t;
    }


    // Euler angles rotations (intrinsic rotations around the axes of the reference system, applied in order Z, Y, X)
    /// Generates a rotation around the X axis
    Transformation R_x(float angle_rad) {
        Transformation t;
        float c = std::cos(angle_rad);
        float s = std::sin(angle_rad);
        // M
        // Already has 1 as mat[0]
        t.m.mat[5] = c;  t.m.mat[6] = -s;
        t.m.mat[9] = s;  t.m.mat[10] = c;
        // Inverse of a rotation matrix is its transpose (or a rotation by -angle)
        t.invm.mat[5] = c;  t.invm.mat[6] = s;
        t.invm.mat[9] = -s; t.invm.mat[10] = c;
        return t;
    }

    /// Generates a rotation around the Y axis
    Transformation R_y(float angle_rad) {
        Transformation t;
        float c = std::cos(angle_rad);
        float s = std::sin(angle_rad);
        // M
        // already has 1 in mat[5]
        t.m.mat[0] = c;  t.m.mat[2] = s;
        t.m.mat[8] = -s; t.m.mat[10] = c;
        // Inverse
        t.invm.mat[0] = c; t.invm.mat[2] = -s;
        t.invm.mat[8] = s; t.invm.mat[10] = c;
        return t;
    }

    /// Generates a rotation around the Z axis
    Transformation R_z(float angle_rad) {
        Transformation t;
        float c = std::cos(angle_rad);
        float s = std::sin(angle_rad);
        // M
        // already has 1 in mat[10]
        t.m.mat[0] = c;  t.m.mat[1] = -s;
        t.m.mat[4] = s;  t.m.mat[5] = c;
        // Inverse
        t.invm.mat[0] = c;  t.invm.mat[1] = s;
        t.invm.mat[4] = -s; t.invm.mat[5] = c;
        return t;
    }

    // RP: I think we will need some (angle, axis) type rotation/reflection transformation. We will see in lectures I guess.
};

// ===================================================================================
// ===================================================================================
// METHODS
// ===================================================================================
// ===================================================================================

// ================================================
// Methods to compute length in Vec and Normal
// ================================================

template<typename Curr> float norm2 (const Curr& left) {
    return left.x * left.x + left.y * left.y + left.z * left.z;
}

template<typename Curr> float norm (const Curr& left) {
    return std::sqrt(left.x * left.x + left.y * left.y + left.z * left.z);
}

// ======================================================
// Methods to compute and access length in Vec and Normal
// ======================================================

float Vec::norm() const { return ::norm<Vec>(*this); }
float Vec::norm2() const { return ::norm2<Vec>(*this); }

float Normal::norm() const { return ::norm<Normal>(*this); }
float Normal::norm2() const { return ::norm2<Normal>(*this); }

float Point::norm() const { return ::norm<Point>(*this); }
float Point::norm2() const { return ::norm2<Point>(*this); }

/// Return a normalized Vec (a Vec with the same direction but length 1)
Vec Vec::normalize() const {
    return _scalar_divide<Vec, float, Vec>(*this, this->norm());
}

/// Renormalize a Normal which is not guaranteed to be of length 1 (rounding, ecc.)
Normal Normal::normalize() {
    return _scalar_divide<Normal, float, Normal>(*this, this->norm());
}

Normal Point::to_norm() const {
    return _scalar_divide<Point, float, Normal>(*this, this->norm());
}

// ======================================================
// is_close methods
// ======================================================

bool Point::is_close(const Point& other, float epsilon) const {
    return aux::are_xyz_close(*this, other, epsilon);
}

bool Vec::is_close(const Vec& other, float epsilon) const {
    return aux::are_xyz_close(*this, other, epsilon);
}

bool Normal::is_close(const Normal& other, float epsilon) const {
    return aux::are_xyz_close(*this, other, epsilon);
}

bool HomMatrix::is_close(const HomMatrix& other, float epsilon) const {
    for (int i = 0; i < 16; ++i) {
        if (!aux::are_close(mat[i], other.mat[i], epsilon)) return false;
    }
    return true;
}

// ================================================
// Point to vec , Vec to Normal
// ================================================

/// Returns a Vec with the same components as the Point (but different type)
Vec Point::to_vec() const {
    return _same<Point, Vec>(*this);
}

/// Returns a Normal with the same direction as the Vec
Normal Vec::to_norm() const {
    return _scalar_divide<Vec, float, Normal>(*this, this->norm());
}


/// Transformation consistency
bool Transformation::is_consistent() const {
    // Exploit M*M multiplication
    HomMatrix result = m * invm;
    HomMatrix identity; // Default is identity

    return result.is_close(identity);
}

/// Inversion
Transformation Transformation::inverse() const {
    // Crea una nuova Transformation scambiando mat e invm!
    return Transformation{invm, m};
}

// ============================================================
// std::formatter struct for Point, Vec, Normal and HomMatrix
// ============================================================

// Formatting via context and custom formatter to enable std::format support for Point, Vec and Normal (and HomMatrix)
// For example, std::stirng s = std::format("Point({:.2f})", Point{1.0f, 2.0f, 3.0f}) will produce the string "Point(1.00, 2.00, 3.00)"
export template <>
struct std::formatter<Point> {
    std::formatter<float> float_fmt;

    constexpr auto parse(std::format_parse_context& ctx) {
        return float_fmt.parse(ctx);
    }

    auto format(const Point& p, auto& ctx) const {
        auto it = ctx.out();
        it = float_fmt.format(p.x, ctx);
        it = std::format_to(it, " ");
        ctx.advance_to(it);
        it = float_fmt.format(p.y, ctx);
        it = std::format_to(it, " ");
        ctx.advance_to(it);
        return float_fmt.format(p.z, ctx);
    }
};

export template <>
struct std::formatter<Vec> {
    std::formatter<float> float_fmt;

    constexpr auto parse(std::format_parse_context& ctx) {
        return float_fmt.parse(ctx);
    }

    auto format(const Vec& v, auto& ctx) const {
        auto it = ctx.out();
        it = float_fmt.format(v.x, ctx);
        it = std::format_to(it, " ");
        ctx.advance_to(it);
        it = float_fmt.format(v.y, ctx);
        it = std::format_to(it, " ");
        ctx.advance_to(it);
        return float_fmt.format(v.z, ctx);
    }
};

export template <>
struct std::formatter<Normal> {
    std::formatter<float> float_fmt;

    constexpr auto parse(std::format_parse_context& ctx) {
        return float_fmt.parse(ctx);
    }

    auto format(const Normal& n, auto& ctx) const {
        auto it = ctx.out();
        it = float_fmt.format(n.x, ctx);
        it = std::format_to(it, " ");
        ctx.advance_to(it);
        it = float_fmt.format(n.y, ctx);
        it = std::format_to(it, " ");
        ctx.advance_to(it);
        return float_fmt.format(n.z, ctx);
    }
};

export template <>
struct std::formatter<HomMatrix> {
    std::formatter<float> float_fmt;

    constexpr auto parse(std::format_parse_context& ctx) {
        return float_fmt.parse(ctx);
    }

    auto format(const HomMatrix& M, auto& ctx) const {
        auto it = ctx.out();
        for (int i = 0; i < 16; ++i) {
            it = float_fmt.format(M.mat[i], ctx);
            if ((i + 1) % 4 == 0) {
                it = std::format_to(it, "\n");
            } else {
                it = std::format_to(it, " ");
            }
            ctx.advance_to(it);
        }
        return it;
    }
};