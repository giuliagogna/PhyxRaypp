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
 * @brief Core geometric primitives, vector algebra, and spatial transformations.[cite: 1]
 *
 * This module defines points, vectors, normals, homogeneous matrices,
 * and affine transformations used throughout the renderer.[cite: 1]
 *
 * The distinction between Point, Vec, and Normal is intentional and helps
 * prevent invalid geometric operations at compile time.[cite: 1]
 */

module;

//#include <format>

export module Geometry;

import auxiliary_functions;
import std;

/// @brief 2D coordinates used for parametric surface and texture coordinates.[cite: 1]
export struct Vec2D {
    float u{0.0f}, v{0.0f};
    /// @brief Check if two 2D vectors are close enough within an epsilon[cite: 1]
    bool is_close(const Vec2D other, float epsilon = 1e-5f) const;
};

/// @brief Unit-length direction vector used for orientations and shading.[cite: 1]
export struct Normal {
    float x{0.0f}, y{0.0f}, z{0.0f};

    /// @brief Compute length of Normal object[cite: 1]
    float norm() const;  
    /// @brief Compute length square of Normal object[cite: 1]
    float norm2() const; 

    /// @brief Normalizes the Normal object (non-const)[cite: 1]
    Normal normalize(); 

    /// @brief Check if two Normals are close enough within an epsilon[cite: 1]
    bool is_close(const Normal other, float epsilon = 1e-5f) const; 
};

/// @brief 3D displacement or direction vector.[cite: 1]
export struct Vec {
    float x{0.0f}, y{0.0f}, z{0.0f};

    /// @brief Pure conversion without implicit and expensive hidden normalization. 
    /// It has to be performed before.
    Normal to_norm() const; 
    
    /// @brief Normalizes and returns a Vec[cite: 1]
    Vec normalize() const; 

    /// @brief Compute length of Vec[cite: 1]
    float norm() const;
    /// @brief Compute length square of Vec[cite: 1]
    float norm2() const;

    /// @brief Check if two Vec are close enough within an epsilon[cite: 1]
    bool is_close(const Vec other, float epsilon = 1e-5f) const;
};

/// @brief 3D position in space.[cite: 1]
export struct Point {
    float x{0.0}, y{0.0}, z{0.0};
    
    /// @brief Converts a Point to a Vec[cite: 1]
    Vec to_vec() const;
    
    /// @brief Check if two Points are close enough within an epsilon[cite: 1]
    bool is_close(const Point other, float epsilon = 1e-5f) const;
};

/// @brief 4x4 homogeneous transformation matrix.[cite: 1]
/// This is only the basic object that stores a 4x4 Homogeneous Matrix (inverse matrix
/// and consistency checks are implemented inside Transformation struct)[cite: 1]
export struct HomMatrix {
    std::array<float, 16> mat = {1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f};

    /// @brief Checks if two matrixes are close enough within an epsilon
    bool is_close(const HomMatrix& other, float epsilon = 1e-5f) const;
};

/// @brief Affine transformation storing both a matrix and its inverse.[cite: 1]
/// It stores the direct matrix of the transformation and its inverse: when applying transformation to a
/// Point or Vec one should use the direct matrix.[cite: 1]
/// One often wants to apply the inverse transformation: to do so we implement a method
/// `inverse()` that simply switches the two matrixes, so that now the inverse is the direct and vice-versa.[cite: 1]
export struct Transformation {
    HomMatrix m;
    HomMatrix invm;

    bool is_consistent() const;

    /// @brief Transformation inversion:
    /// Creates a new transformation exchanging the matrix and the inverse[cite: 1]
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
    /// @brief Point += Vec -> Point[cite: 1]
    [[gnu::always_inline]] inline Point& operator+= (Point& p, Vec v) {
        p.x += v.x; p.y += v.y; p.z += v.z;
        return p;
    }

    /// @brief Point + Vec -> Point[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Point operator+ (Point p, Vec v) {
        return Point{ p.x + v.x, p.y + v.y, p.z + v.z };
    }

    /// @brief Vec += Vec -> Vec[cite: 1]
    [[gnu::always_inline]] inline Vec& operator+= (Vec& v, Vec other) {
        v.x += other.x; v.y += other.y; v.z += other.z;
        return v;
    }

    /// @brief Vec + Vec -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator+ (Vec v, Vec other) {
        return Vec{ v.x + other.x, v.y + other.y, v.z + other.z };
    }

    // Differences
    /// @brief Point -= Vec -> Point[cite: 1]
    [[gnu::always_inline]] inline Point& operator-= (Point& p, Vec v) {
        p.x -= v.x; p.y -= v.y; p.z -= v.z;
        return p;
    }

    /// @brief Point - Vec -> Point[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Point operator- (Point p, Vec v) {
        return Point{ p.x - v.x, p.y - v.y, p.z - v.z };
    }

    /// @brief Vec -= Vec -> Vec[cite: 1]
    [[gnu::always_inline]] inline Vec& operator-= (Vec& v, Vec other) {
        v.x -= other.x; v.y -= other.y; v.z -= other.z;
        return v;
    }

    /// @brief Vec - Vec -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator- (Vec v, Vec other) {
        return Vec{ v.x - other.x, v.y - other.y, v.z - other.z };
    }

    /// @brief Point - Point -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator- (Point p, Point other) {
        return Vec{ p.x - other.x, p.y - other.y, p.z - other.z };
    }

    // Negations
    /// @brief -Vec -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator- (Vec v) {
        return Vec{ -v.x, -v.y, -v.z };
    }

    /// @brief -Point -> Point[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Point operator- (Point p) {
        return Point{ -p.x, -p.y, -p.z };
    }

    /// @brief -Normal -> Normal[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Normal operator- (Normal n) {
        return Normal{ -n.x, -n.y, -n.z };
    }
    
    // Scalar products
    /// @brief Point *= scalar -> Point[cite: 1]
    [[gnu::always_inline]] inline Point& operator*= (Point& p, float scalar) {
        p.x *= scalar; p.y *= scalar; p.z *= scalar;
        return p;
    }

    /// @brief Point * scalar -> Point[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Point operator* (Point p, float scalar) {
        return Point{ p.x * scalar, p.y * scalar, p.z * scalar };
    }

    /// @brief Scalar * Point -> Point[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Point operator* (float scalar, Point p) {
        return Point{ p.x * scalar, p.y * scalar, p.z * scalar };
    }

    /// @brief Vec *= scalar -> Vec[cite: 1]
    [[gnu::always_inline]] inline Vec& operator*= (Vec& v, float scalar) {
        v.x *= scalar; v.y *= scalar; v.z *= scalar;
        return v;
    }

    /// @brief Vec * scalar -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator* (Vec v, float scalar) {
        return Vec{ v.x * scalar, v.y * scalar, v.z * scalar };
    }

    /// @brief Scalar * Vec -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator* (float scalar, Vec v) {
        return Vec{ v.x * scalar, v.y * scalar, v.z * scalar };
    }

    /// @brief Normal * scalar -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator* (Normal n, float scalar) {
        return Vec{ n.x * scalar, n.y * scalar, n.z * scalar };
    }

    /// @brief Scalar * Normal -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator* (float scalar, Normal n) {
        return Vec{ n.x * scalar, n.y * scalar, n.z * scalar };
    }

    // Scalar division
    /// @brief Vec /= scalar -> Vec[cite: 1]
    [[gnu::always_inline]] inline Vec& operator/= (Vec& v, float scalar) {
        float inv = 1.0f / scalar;
        v.x *= inv; v.y *= inv; v.z *= inv;
        return v;
    }

    /// @brief Vec / scalar -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator/ (Vec v, float scalar) {
        float inv = 1.0f / scalar;
        return Vec{ v.x * inv, v.y * inv, v.z * inv };
    }

    /// @brief Normal / scalar -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator/ (Normal n, float scalar) {
        float inv = 1.0f / scalar;
        return Vec{ n.x * inv, n.y * inv, n.z * inv };
    }

    /// @brief Point / scalar -> Point[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Point operator/ (Point p, float scalar) {
        float inv = 1.0f / scalar;
        return Point{ p.x * inv, p.y * inv, p.z * inv };
    }

    // Dot products
    /// @brief Dot product between two Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline float operator* (Vec v, Vec other) {
        return v.x * other.x + v.y * other.y + v.z * other.z;
    }

    /// @brief Dot products between a Vec and a Normal[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline float operator* (Vec v, Normal n) {
        return v.x * n.x + v.y * n.y + v.z * n.z;
    }

    /// @brief Dot products between a Normal and a Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline float operator* (Normal n, Vec v) {
        return n.x * v.x + n.y * v.y + n.z * v.z;
    }

    /// @brief Dot product between two Normal[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline float operator* (Normal v, Normal n) {
        return v.x * n.x + v.y * n.y + v.z * n.z;
    }

    // Cross products
    /// @brief Cross product between two Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator% (Vec v, Vec other) {
        return Vec{
            v.y * other.z - v.z * other.y,
            v.z * other.x - v.x * other.z,
            v.x * other.y - v.y * other.x
        };
    }

    /// @brief Cross product between a Vec and a Normal[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator% (Vec v, Normal n) {
        return Vec{
            v.y * n.z - v.z * n.y,
            v.z * n.x - v.x * n.z,
            v.x * n.y - v.y * n.x
        };
    }

    /// @brief Cross product between a Normal and a Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator% (Normal n, Vec v) {
        return Vec{
            n.y * v.z - n.z * v.y,
            n.z * v.x - n.x * v.z,
            n.x * v.y - n.y * v.x
        };
    }

    /// @brief Cross product between two Normal[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator% (Normal n, Normal other) {
        return Vec{
            n.y * other.z - n.z * other.y,
            n.z * other.x - n.x * other.z,
            n.x * other.y - n.y * other.x
        };
    }

    // Matrix multiplication
    /// @brief Matrix * Point -> Point[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Point operator* (const HomMatrix& M, Point p) {
        return Point{
            M.mat[0] * p.x + M.mat[1] * p.y + M.mat[2] * p.z + M.mat[3],
            M.mat[4] * p.x + M.mat[5] * p.y + M.mat[6] * p.z + M.mat[7],
            M.mat[8] * p.x + M.mat[9] * p.y + M.mat[10] * p.z + M.mat[11]
        };
    }

    /// @brief Matrix * Vec -> Vec[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator* (const HomMatrix& M, Vec v) {
        return Vec{
            M.mat[0] * v.x + M.mat[1] * v.y + M.mat[2] * v.z,
            M.mat[4] * v.x + M.mat[5] * v.y + M.mat[6] * v.z,
            M.mat[8] * v.x + M.mat[9] * v.y + M.mat[10] * v.z
        };
    }

    // ================================================
    // MATRIX OPERATIONS
    // ================================================

    /// @brief Matrix * Matrix -> Matrix[cite: 1]
    // Not forcing inlining since this operator is usually called
    // creating transformations to pass to constructors
    [[nodiscard]] inline HomMatrix operator* (const HomMatrix& M1, const HomMatrix& M2) {
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

            0.0f, 0.0f, 0.0f, 1.0f // last row of a hom. matrix optimized for affine transforms
        };
    }

    // ================================================
    // OPERATIONS ON TRANSFORMATIONS
    // ================================================

    /// @brief Transformation composition[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Transformation operator*(const Transformation& T1, const Transformation& T2) {
        return Transformation{
            T1.m * T2.m,
            T2.invm * T1.invm
        };
    }

    /// @brief Transformation of a Point (By value)[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Point operator*(const Transformation& T, Point p) {
        return T.m * p;
    }

    /// @brief Transformation of a Vec (By value)[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Vec operator*(const Transformation& T, Vec v) {
        return T.m * v;
    }

    /// @brief Transformation of a Normal (By value)[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Normal operator* (const Transformation& T, Normal n) {
        return Normal{
            T.invm.mat[0] * n.x + T.invm.mat[4] * n.y + T.invm.mat[8] * n.z,
            T.invm.mat[1] * n.x + T.invm.mat[5] * n.y + T.invm.mat[9] * n.z,
            T.invm.mat[2] * n.x + T.invm.mat[6] * n.y + T.invm.mat[10] * n.z
        };
    }
    
    // =============================================================
    // Component-wise min/max utilities used for AABB construction[cite: 1]
    // =============================================================

    /// @brief Compute the Point with coordinates the minimum among the coordinates of the two input points[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Point min (Point left, Point right) {
        return Point{ std::min(left.x, right.x), std::min(left.y, right.y), std::min(left.z, right.z) };
    }

    /// @brief Compute the Point with coordinates the maximum among the coordinates of the two input points[cite: 1]
    [[nodiscard]] [[gnu::always_inline]] inline Point max (Point left, Point right) {
        return Point{ std::max(left.x, right.x), std::max(left.y, right.y), std::max(left.z, right.z) };
    }

    // ================================================
    // TRANSFORMATION GENERATORS
    // ================================================

    /** @brief Create a translation transformation.[cite: 1] */
    /// (Optimized by value)
    inline Transformation Trans(Vec v) {
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

    /** @brief Create a non-uniform scaling transformation.[cite: 1] */
    /// (Optimized by value)
    inline Transformation Scale(Vec v) {
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
    /** @brief Create a rotation around the X axis (radians).[cite: 1] */
    inline Transformation R_x(float angle_rad) {
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

    /** @brief Create a rotation around the Y axis (radians).[cite: 1] */
    inline Transformation R_y(float angle_rad) {
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

    /** @brief Create a rotation around the Z axis (radians).[cite: 1] */
    inline Transformation R_z(float angle_rad) {
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
};

// ===================================================================================
// ===================================================================================
// METHODS
// ===================================================================================
// ===================================================================================

// ======================================================
// Methods to compute and access length in Vec and Normal
// ======================================================

[[nodiscard]] [[gnu::always_inline]] inline float Vec::norm2() const { 
    return x * x + y * y + z * z; 
}

[[nodiscard]] [[gnu::always_inline]] inline float Vec::norm() const { 
    return std::sqrt(x * x + y * y + z * z); 
}

[[nodiscard]] [[gnu::always_inline]] inline float Normal::norm2() const { 
    return x * x + y * y + z * z; 
}

[[nodiscard]] [[gnu::always_inline]] inline float Normal::norm() const { 
    return std::sqrt(x * x + y * y + z * z); 
}

/// @brief Return a normalized Vec (Fast inverse square root or standard division)[cite: 1]
[[nodiscard]] [[gnu::always_inline]] inline Vec Vec::normalize() const {
    float n = std::sqrt(x * x + y * y + z * z);
    float inv = 1.0f / n;
    return Vec{ x * inv, y * inv, z * inv };
}

/// @brief Renormalize a Normal[cite: 1]
[[gnu::always_inline]] inline Normal Normal::normalize() {
    float n = std::sqrt(x * x + y * y + z * z);
    float inv = 1.0f / n;
    x *= inv; y *= inv; z *= inv;
    return *this;
}

// ======================================================
// is_close methods
// ======================================================

[[nodiscard]] [[gnu::always_inline]] inline bool Vec2D::is_close(Vec2D other, float epsilon) const {
    return aux::are_close(u, other.u, epsilon) &&
           aux::are_close(v, other.v, epsilon);
}

[[nodiscard]] [[gnu::always_inline]] inline bool Point::is_close(Point other, float epsilon) const {
    return aux::are_close(x, other.x, epsilon) &&
           aux::are_close(y, other.y, epsilon) &&
           aux::are_close(z, other.z, epsilon);
}

[[nodiscard]] [[gnu::always_inline]] inline bool Vec::is_close(Vec other, float epsilon) const {
    return aux::are_close(x, other.x, epsilon) &&
           aux::are_close(y, other.y, epsilon) &&
           aux::are_close(z, other.z, epsilon);
}

[[nodiscard]] [[gnu::always_inline]] inline bool Normal::is_close(Normal other, float epsilon) const {
    return aux::are_close(x, other.x, epsilon) &&
           aux::are_close(y, other.y, epsilon) &&
           aux::are_close(z, other.z, epsilon);
}

[[nodiscard]] inline bool HomMatrix::is_close(const HomMatrix& other, float epsilon) const {
    for (int i = 0; i < 16; ++i) {
        if (!aux::are_close(mat[i], other.mat[i], epsilon)) return false;
    }
    return true;
}

// ================================================
// Point to vec , Vec to Normal
// ================================================

/// @brief Returns a Vec with the same components as the Point (but different type)[cite: 1]
[[nodiscard]] [[gnu::always_inline]] inline Vec Point::to_vec() const {
    return Vec{ x, y, z };
}

/// @brief Pure conversion without implicit and expensive hidden normalization.
/// It has to be performed before.
[[nodiscard]] [[gnu::always_inline]] inline Normal Vec::to_norm() const {
    return Normal{ x, y, z };
}

// ================================================
// Transformation utils
// ================================================

/// @brief Transformation consistency[cite: 1]
[[nodiscard]] inline bool Transformation::is_consistent() const {
    HomMatrix result = m * invm;
    HomMatrix identity; // Default constructor sets identity
    return result.is_close(identity);
}

/// @brief Trasformation inversion:
/// Creates a new transformation exchanging the matrix and the inverse[cite: 1]
[[nodiscard]] [[gnu::always_inline]] inline Transformation Transformation::inverse() const {
    return Transformation{ invm, m };
}

// ============================================================
// std::formatter struct for Point, Vec, Normal and HomMatrix
// ============================================================

/**
 * @brief Custom formatter specializations enabling std::format() support
 * for the geometry types defined in this module.[cite: 1]
 *
 * The formatting specification used for float values is propagated
 * to all components. For example:[cite: 1]
 *
 * std::format("{:.2f}", Point{1.f, 2.f, 3.f})[cite: 1]
 *
 * produces:[cite: 1]
 *
 * 1.00 2.00 3.00[cite: 1]
 */

/// @brief Enable std::format() support for Point.[cite: 1]
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

/// @brief Enable std::format() support for Vec.[cite: 1]
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

/// @brief Enable std::format() support for Normal.[cite: 1]
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

/// @brief Enable std::format() support for HomMatrix.[cite: 1]
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
// ORTHONORMAL BASIS GENERATOR (Zero Overhead)
// ================================================

/**
 * @brief Build an orthonormal basis from a normalized direction.[cite: 1]
 *
 * The input vector becomes the local z-axis of the generated basis.[cite: 1]
 *
 * @tparam VectorType Vec or Normal.[cite: 1]
 * @param normal Normalized direction.[cite: 1]
 * @return std::tuple containing {e1, e2, e3}.[cite: 1]
 */
/// Returns a std::tuple to allow the compiler to use CPU registers directly instead of stack memory.
/// To unpack use: auto [e1, e2, e3] = create_onb_from_z(normal);
export template <typename VectorType>
[[nodiscard]] [[gnu::always_inline]] inline std::tuple<Vec, Vec, Vec> create_onb_from_z(VectorType normal) {
    
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

    return std::make_tuple(e1, e2, e3);
};