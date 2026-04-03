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

import std;


// GG: need to define in this order for dependencies (Vec needs Normal, Point needs Vec)
export struct Normal {
    float x{0.0f}, y{0.0f}, z{0.0f};

    // Compute length of Normal object
    float norm() const;
    float norm2() const;

    Normal normalize(); // normalizes the Normal object
};

export struct Vec {
    float x{0.0f}, y{0.0f}, z{0.0f};
    Normal to_norm() const; // normalizes and returns a Normal
    Vec normalize() const; // normalizes and returns a Vec

    // Compute length and length square
    float norm() const;
    float norm2() const;
};

export struct Point {
    float x{0.0}, y{0.0}, z{0.0};
    Vec to_vec() const; // 
};


// Homogeneous 4x4 Matrix;
export struct HomMatrix {

    std::array<float, 16> mat = {1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f};

    std::array<float, 16> invmat = {1.0f, 0.0f, 0.0f, 0.0f,
                                    0.0f, 1.0f, 0.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 0.0f, 1.0f};
    
    bool isConsistent();    // Verifies if invmat is the inverse of mat
};


// ================================================
// Functions used in conv_to_string
// ================================================

// RP: It's more user-dependent if it wants to put a type label on what it's doing.
//     It can put in the context window something like print("Point: {}", point).
//const char* _get_name(const Point&) { return "Point"; }
//const char* _get_name(const Vec&)   { return "Vec"; }
//const char* _get_name(const Normal&)  { return "Normal"; }


// ================================================
// std::formatter struct for Point, Vec, Normal and HomMatrix
// ================================================

// Formatting via context and custom formatter to enable std::format support for Point, Vec and Normal (and HomMatrix)
// For example, std::stirng s = std::format("Point({:.2f})", Point{1.0f, 2.0f, 3.0f}) will produce the string "Point(1.00, 2.00, 3.00)"
export template <>
struct std::formatter<Point> {
    std::formatter<float> float_fmt;

    constexpr auto parse(std::format_parse_context& ctx) {
        return float_fmt.parse(ctx);
    }

    auto format(const Point& p, std::format_context& ctx) const {
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

    auto format(const Vec& v, std::format_context& ctx) const {
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

    auto format(const Normal& n, std::format_context& ctx) const {
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

    auto format(const HomMatrix& M, std::format_context& ctx) const {
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

template<typename L, typename R, typename Res> Res _dot_product (const L& left, const R& right) {
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
// Methods to compute length in Vec and Normal
// ================================================

// RP: I remove the "_" since I think this will be used out of the struct as well

// GG: Added calculation of norm squared
// (it's much faster to calculate than norm: avoid calculation of norm squared trough a sqrt and then ^2)
template<typename Curr> float norm2 (const Curr& left) {
    return left.x * left.x + left.y * left.y + left.z * left.z;
}

template<typename Curr> float norm (const Curr& left) {
    return std::sqrt(left.x * left.x + left.y * left.y + left.z * left.z);
}

// ======================================================
// Methods to compute and access length in Vec and Normal
// ======================================================

float Vec::norm() const { return norm<Vec>(*this); }
float Vec::norm2() const { return norm2<Vec>(*this); }

float Normal::norm() const { return norm<Normal>(*this); }
float Normal::norm2() const { return norm2<Normal>(*this); }

// GG: I also want a method that takes a Vec and returns a Vec of length 1 (if we need to sum, we can
// 2 Vecs, not a Vec and a Normal)

// RP: I feel this is like the same problem of not having Vec + Point -> Vec. If we normalize something
//     For this purpose we have the Vec::to_norm. I'll leave this method but I'm not sure it's safe to use.

// RP: I feel it's faster to don't store the norm in a variable and then pass
//     by copy. We just pass the method as argument. However, it's possible that the compiler
//     optimizes the code putting the variable in a register so it could generare just the same binary code.
//     I actaully prefer the version without the variable since it's readable anc it's just one line :-)

//Vec Vec::normalize() const {
//    float length = this->norm(); // GG: Use method instead of function
//    return _scalar_divide<Vec, float, Vec>(*this, length);
//}

/// Return a normalized Vec (a Vec with the same direction but length 1)
Vec Vec::normalize() const {
    return _scalar_divide<Vec, float, Vec>(*this, this->norm());
}


// RP: I prefer this to be a non-const method, following the lecture guidelines.

/// Renormalize a Normal which is not guaranteed to be of length 1 (rounding, ecc.)
Normal Normal::normalize() {
    return _scalar_divide<Normal, float, Normal>(*this, this->norm());
}


// ================================================
// Point to vec , Vec to Normal
// ================================================

// Returns a Vec with the same components as the Point (but different type)
Vec Point::to_vec() const {
    return _same<Point, Vec>(*this);
}

// Returns a Normal with the same direction as the Vec
Normal Vec::to_norm() const {
    // GG: Vec does not have a norm method to invoke: use norm function defined above
    // GG: used _scalar_divide template function (operator / or * return a Vec, I want Normal)

//    float length = this->norm(); // GG: Use method instead of function
//    return _scalar_divide<Vec, float, Normal>(*this, length);

    return _scalar_divide<Vec, float, Normal>(*this, this->norm());
}



// ================================================
// OPERATORS OVERLOAD
// ================================================

export {

    // Sums
    /// Point += Vec -> Point
    Point& operator+= (Point& p, const Vec& v) {
        p = _sum<Point, Vec, Point>(p, v);
        return p;
    }

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

    /// Point - Vec -> Point
    Point& operator-= (Point& p, const Vec& v) {
        p = _difference<Point, Vec, Point>(p, v);
        return p;
    }

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

    // GG: It does not make sense mathematically to do Vec - Point
    // (cannot subtract a position from a direction while it does make sense to subtract a direction (Vec) from a position
    // and return a new direction (Vec))
    //Point operator- (const Vec& v, const Point& p) {
    //    return _difference<Vec, Point, Point>(v, p);
    //}

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

    // Scalar products

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

    /// Vec * scalar -> Vec
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

    // Dot products

    // GG: It does not make sense to do the scalar product between a Point and a Normal (direction)
    // Dot products are used to find the projections of a direction (a Vec or Normal) on another or the angle
    // between them, so it does not make sense to try to make the dot product between a position and a direction
    //float operator* (const Point& p, const Normal& n) {
    //    auto res = _dot_product<Point, Normal, Vec>(p, n);
    //    return res.x + res.y + res.z;
    //}

    /// Dot product between two Vec
    float operator* (const Vec& v, const Vec& other) {
        auto res = _dot_product<Vec, Vec, Vec>(v, other);
        return res.x + res.y + res.z;
    }
    /// Dot products between a Vec and a Normal
    float operator* (const Vec& v, const Normal& n) {
        auto res = _dot_product<Vec, Normal, Vec>(v, n);
        return res.x + res.y + res.z;
    }
    /// Dot product between two Normal
    float operator* (const Normal& v, const Normal& n) {
        auto res = _dot_product<Normal, Normal, Vec>(v, n);
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

    /// Scalar division between a Normal and a scalar
    Normal& operator/= (Normal& n, float scalar) {
        n = _scalar_divide<Normal, float, Normal>(n, scalar);
        return n;
    }

    /// Scalar division between a Normal and a scalar
    Normal operator/ (const Normal& n, float scalar) {
        return _scalar_divide<Normal, float, Normal>(n, scalar);
    }

    // needed for homogeneous division in Point operator* (const HomMatrix& M, const Point& p)
    /// Scalar division between a Point and a scalar
    Point operator/ (const Point& p, float scalar) {
        return _scalar_divide<Point, float, Point>(p, scalar);
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
//        if (w==1.f) {
//            return res;
//        }
        
        return res / w; // homogeneous division
    }

    Vec operator* (const HomMatrix& M, const Vec& v) {
        return Vec{
            M.mat[0] * v.x + M.mat[1] * v.y + M.mat[2] * v.z,
            M.mat[4] * v.x + M.mat[5] * v.y + M.mat[6] * v.z,
            M.mat[8] * v.x + M.mat[9] * v.y + M.mat[10] * v.z
        };
    }

    // Traspose of the inverse of the homogeneous matrix is used to transform normals
    Normal operator* (const HomMatrix& M, const Normal& n) {
        return Normal{
            M.invmat[0] * n.x + M.invmat[4] * n.y + M.invmat[8] * n.z,
            M.invmat[1] * n.x + M.invmat[5] * n.y + M.invmat[9] * n.z,
            M.invmat[2] * n.x + M.invmat[6] * n.y + M.invmat[10] * n.z
        };
    }

    // RP: seems that this is optimized by the compiler.
    HomMatrix operator*(const HomMatrix& M1, const HomMatrix& M2) {
        HomMatrix res;
        for (int i = 0; i < 4; ++i) {
            for (int k = 0; k < 4; ++k) {
                float s = M1.mat[i * 4 + k];
                for (int j = 0; j < 4; ++j) {
                    res.mat[i * 4 + j] += s * M2.mat[k * 4 + j];
                }
            }
        }
        return res;
    }

    // Conversion to string
    // Point, Vec, Normal

    // RP: Note that in this way we can no more use the context {} to print a Vec.
    // If we want to print a Vec with 2 decimal places we cannot use {:.2f} as context,
    // but we have to access each components, which is basically like not having any printing function at all.
//    template<typename Obj> std::string conv_to_string(const Obj &obj) {
//        return std::format("{}({:.2f}, {:.2f}, {:.2f})",
//                           _get_name(obj), obj.x, obj.y, obj.z);
//    }
//
//    // Matrix
//    std::string conv_to_string(HomMatrix M) {
//
//        return std::format(
//            "HomMatrix(\n"
//            "  [{:.2f}, {:.2f}, {:.2f}, {:.2f}]\n"
//            "  [{:.2f}, {:.2f}, {:.2f}, {:.2f}]\n"
//            "  [{:.2f}, {:.2f}, {:.2f}, {:.2f}]\n"
//            "  [{:.2f}, {:.2f}, {:.2f}, {:.2f}]\n"
//            ")",
//            M.mat[0], M.mat[1], M.mat[2], M.mat[3],
//            M.mat[4], M.mat[5], M.mat[6], M.mat[7],
//            M.mat[8], M.mat[9], M.mat[10], M.mat[11],
//            M.mat[12], M.mat[13], M.mat[14], M.mat[15]
//        );
//
//    }

};