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
 * @file Geometry.cppm
 * @brief Core geometric primitives, vector algebra, and spatial transformations.
 *
 * This module defines points, vectors, normals, homogeneous matrices,
 * and affine transformations used throughout the renderer.
 *
 * The distinction between Point, Vec, and Normal is intentional and helps
 * prevent invalid geometric operations at compile time.
 */

module;

export module Geometry;

import auxiliary_functions;
import std;

/// @brief 2D coordinates used for parametric surface and texture coordinates.
export struct Vec2D {
    float u{0.0f}, v{0.0f};
    /// @brief Check if two 2D vectors are close enough within an epsilon
    bool is_close(const Vec2D& other, float epsilon = 1e-5f) const;
};

/// @brief Unit-length direction vector used for orientations and shading.
export struct Normal {
    float x{0.0f}, y{0.0f}, z{0.0f};

    /// @brief Compute length of Normal object
    float norm() const;
    /// @brief Compute length square of Normal object
    float norm2() const;

    /// @brief Normalizes the Normal object (non-const)
    Normal normalize();

    /// @brief Check if two Normals are close enough within an epsilon
    bool is_close(const Normal& other, float epsilon = 1e-5f) const;
};

/// @brief 3D displacement or direction vector.
export struct Vec {
    float x{0.0f}, y{0.0f}, z{0.0f};

    /// @brief Normalizes a Vec and returns a Normal object
    Normal to_norm() const;
    /// @brief Normalizes and returns a Vec
    Vec normalize() const;

    /// @brief Compute length of Vec
    float norm() const;
    /// @brief Compute length square of Vec
    float norm2() const;

    /// @brief Check if two Vec are close enough within an epsilon
    bool is_close(const Vec& other, float epsilon = 1e-5f) const;
};

/// @brief 3D position in space.
export struct Point {
    float x{0.0}, y{0.0}, z{0.0};
    /// @brief Converts a Point to a Vec
    Vec to_vec() const;
    /// @brief Check if two Points are close enough within an epsilon
    bool is_close(const Point& other, float epsilon = 1e-5f) const;
};

/// @brief 4x4 homogeneous transformation matrix.
/// This is only the basic object that stores a 4x4 Homogeneous Matrix (inverse matrix
/// and consistency checks are implemented inside Transformation struct)
export struct HomMatrix {
    std::array<float, 16> mat = {1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f};

    bool is_close(const HomMatrix& other, float epsilon = 1e-5f) const;
};

/// @brief Affine transformation storing both a matrix and its inverse.
/// It stores the direct matrix of the transformation and its inverse: when applying transformation to a
/// Point or Vec one should use the direct matrix.
/// One often wants to apply the inverse transformation: to do so  we implement a method
/// `inverse()` that simply  switches the two matrixes, so that now the inverse is the direct and vice-versa.
export struct Transformation {
    HomMatrix m;
    HomMatrix invm;

    bool is_consistent() const;

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
    /// Point *= scalar -> Point
    Point operator*= (Point& p, float scalar) {
        // GG: Need to assign the result to the calling object
        p = _scalar_multiply<Point, float, Point>(p, scalar);
        return p;
    }
    /// Point * scalar -> Point
    Point operator* (const Point& p, float scalar) {
        return _scalar_multiply<Point, float, Point>(p, scalar);
    }
    /// Scalar * Point -> Point
    Point operator* (float scalar, const Point& p) {
        return _scalar_multiply<Point, float, Point>(p, scalar);
    }
    /// Vec *= scalar -> Vec
    Vec& operator*= (Vec& v, float scalar) {
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
    /// Vec /= scalar -> Vec
    Vec& operator/= (Vec& v, float scalar) {
        v = _scalar_divide<Vec, float, Vec>(v, scalar);
        return v;
    }
    /// Vec / scalar -> Vec
    Vec operator/ (const Vec& v, float scalar) {
        return _scalar_divide<Vec, float, Vec>(v, scalar);
    }
    /// Normal / scalar -> Vec
    Vec operator/ (const Normal& n, float scalar) {
        return _scalar_divide<Normal, float, Vec>(n, scalar);
    }
    /// Point / scalar -> Point
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
    /// Matrix * Point -> Point
    Point operator* (const HomMatrix& M, const Point& p) {

        float px = p.x; float py = p.y; float pz = p.z; // This should be optimized by the compiler to avoid overhead of multiple accesses to p.x, p.y and p.z

        Point res{
            M.mat[0] * px + M.mat[1] * py + M.mat[2] * pz + M.mat[3],
            M.mat[4] * px + M.mat[5] * py + M.mat[6] * pz + M.mat[7],
            M.mat[8] * px + M.mat[9] * py + M.mat[10] * pz + M.mat[11]
        };

        float w = M.mat[12] * px + M.mat[13] * py + M.mat[14] * pz + M.mat[15];

        if (w==1.f) {
            return res;
        }

        return res / w; // homogeneous division
    }
    /// Matrix * Vec -> Vec
    Vec operator* (const HomMatrix& M, const Vec& v) {
        return Vec{
            M.mat[0] * v.x + M.mat[1] * v.y + M.mat[2] * v.z,
            M.mat[4] * v.x + M.mat[5] * v.y + M.mat[6] * v.z,
            M.mat[8] * v.x + M.mat[9] * v.y + M.mat[10] * v.z
        };
    }
    /// Matrix * Matrix -> Matrix
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

    /** @brief Create a translation transformation. */
    Transformation Trans(const Vec& v) {
        Transformation t; // Starts as Identity
        // Set the last column to the components of the vector
        t.m.mat[3] = v.x;
        t.m.mat[7] = v.y;
        t.m.mat[11] = v.z;
        // Inverse is just a translation by -v
        t.invm.mat[3] = -v.x;
        t.invm.mat[7] = -v.y;
        t.invm.mat[11] = -v.z;
        return t;
    }

    /** @brief Create a non-uniform scaling transformation. */
    Transformation Scale(const Vec& v) {
        Transformation t; // Starts as Identity
        // Set diagonal elements to components of the scaling vector
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
    /** @brief Create a rotation around the X axis (radians). */
    Transformation R_x(float angle_rad) {
        Transformation t;
        float c = std::cos(angle_rad);
        float s = std::sin(angle_rad);
        // Already has 1 as mat[0]
        t.m.mat[5] = c;  t.m.mat[6] = -s;
        t.m.mat[9] = s;  t.m.mat[10] = c;
        // Inverse of a rotation matrix is its transpose (or a rotation by -angle)
        t.invm.mat[5] = c;  t.invm.mat[6] = s;
        t.invm.mat[9] = -s; t.invm.mat[10] = c;
        return t;
    }

    /** @brief Create a rotation around the Y axis (radians). */
    Transformation R_y(float angle_rad) {
        Transformation t;
        float c = std::cos(angle_rad);
        float s = std::sin(angle_rad);
        // Already has 1 in mat[5]
        t.m.mat[0] = c;  t.m.mat[2] = s;
        t.m.mat[8] = -s; t.m.mat[10] = c;
        // Inverse
        t.invm.mat[0] = c; t.invm.mat[2] = -s;
        t.invm.mat[8] = s; t.invm.mat[10] = c;
        return t;
    }

    /** @brief Create a rotation around the Z axis (radians). */
    Transformation R_z(float angle_rad) {
        Transformation t;
        float c = std::cos(angle_rad);
        float s = std::sin(angle_rad);
        // Already has 1 in mat[10]
        t.m.mat[0] = c;  t.m.mat[1] = -s;
        t.m.mat[4] = s;  t.m.mat[5] = c;
        // Inverse
        t.invm.mat[0] = c;  t.invm.mat[1] = s;
        t.invm.mat[4] = -s; t.invm.mat[5] = c;
        return t;
    }

    // =============================================================
    // Component-wise min/max utilities used for AABB construction
    // =============================================================

    /// Return the component-wise minimum of two 3D objects.
    template<typename Curr, typename Res>
    Res min_v(const Curr& left, const Curr& right) {
        return Res{
            std::min(left.x, right.x),
            std::min(left.y, right.y),
            std::min(left.z, right.z)
        };
    }

    /// Return the component-wise maximum of two 3D objects.
    template<typename Curr, typename Res>
    Res max_v(const Curr& left, const Curr& right) {
        return Res{
            std::max(left.x, right.x),
            std::max(left.y, right.y),
            std::max(left.z, right.z)
        };
    }

    /// @brief Compute the Point with coordinates the minimum among the coordinates of
    /// the two input points
    Point min(const Point& left, const Point& right) {
        return min_v<Point, Point>(left, right);
    }

    /// @brief Compute the Point with coordinates the maximum among the coordinates of
    /// the two input points
    Point max(const Point& left, const Point& right) {
        return max_v<Point, Point>(left, right);
    }
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

/// Return a normalized Vec (a Vec with the same direction but length 1)
Vec Vec::normalize() const {
    return _scalar_divide<Vec, float, Vec>(*this, this->norm());
}

/// Renormalize a Normal which is not guaranteed to be of length 1 (rounding, ecc.)
Normal Normal::normalize() {
    return _scalar_divide<Normal, float, Normal>(*this, this->norm());
}

// ======================================================
// is_close methods
// ======================================================

bool Vec2D::is_close(const Vec2D& other, float epsilon) const {
    return aux::are_close(u, other.u, epsilon) &&
           aux::are_close(v, other.v, epsilon);
}

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

/// @brief Trasformation inversion:
/// Creates a new transformation exchanging the matrix and the inverse
Transformation Transformation::inverse() const {
    return Transformation{invm, m};
}

// ============================================================
// std::format support
// ============================================================

/**
 * Custom formatter specializations enabling std::format() support
 * for the geometry types defined in this module.
 *
 * The formatting specification used for float values is propagated
 * to all components. For example:
 *
 * std::format("{:.2f}", Point{1.f, 2.f, 3.f})
 *
 * produces:
 *
 * 1.00 2.00 3.00
 */

/// Enable std::format() support for Point.
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

/// Enable std::format() support for Vec.
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

/// Enable std::format() support for Normal.
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

/// Enable std::format() support for HomMatrix.
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

// ================================================
// TEMPLATE ORTHONORMAL BASIS GENERATOR
// ================================================

/**
 * @brief Build an orthonormal basis from a normalized direction.
 *
 * The input vector becomes the local z-axis of the generated basis.
 *
 * @tparam VectorType Vec or Normal.
 * @param normal Normalized direction.
 * @return Array containing {e1, e2, e3}.
 */
/// Create a orthonormal basis (ONB) from a vector representing the z axis (normalized)
/// Return a tuple containing the three vectors (e1, e2, e3) of the basis. The result is such
/// that e3 = normal.
/// The `normal` vector must be *normalized*, otherwise this method won't work.
export template <typename VectorType>
std::array<Vec, 3> create_onb_from_z(const VectorType& normal) {

    // std::copysign(1.0f, normal.z) copies the sign bit of normal.z onto 1.0f.
    // It returns exactly 1.0f if positive/zero, and -1.0f if negative.
    float sign = std::copysign(1.0f, normal.z);

    float a = -1.0f / (sign + normal.z);
    float b = normal.x * normal.y * a;

    Vec e1{
        1.0f + sign * normal.x * normal.x * a,
        sign * b,
        -sign * normal.x
    };

    Vec e2{
        b,
        sign + normal.y * normal.y * a,
        -normal.y
    };

    Vec e3{
        normal.x,
        normal.y,
        normal.z
    };

    // Returns an array containing the [e1, e2, e3] basis vectors
    // To unpack use auto [e1, e2, e3] = create_onb_from_z()
    return std::array<Vec, 3>{e1, e2, e3};
}