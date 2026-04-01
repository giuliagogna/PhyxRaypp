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

#include <format>

export module Geometry;

import std;


// GG: need to define in this order for dependencies (Vec needs Norm, Point needs Vec)
export struct Norm {
    float x{0.0f}, y{0.0f}, z{0.0f};
};

export struct Vec {
    float x{0.0f}, y{0.0f}, z{0.0f};
    Norm to_norm() const;
};

export struct Point {
    float x{0.0}, y{0.0}, z{0.0};
    Vec to_vec() const;
};


export struct HomMatrix {
    // Homogeneous 4x4 matrix
    std::array<float, 16> mat = {1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f};
};


// Functions used in conv_to_string: not exported (module linkage)
const char* _get_name(const Point&) { return "Point"; }
const char* _get_name(const Vec&)   { return "Vec"; }
const char* _get_name(const Norm&)  { return "Norm"; }


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

template<typename Curr, typename Res> Res _norm (const Curr& left) {
    return std::sqrt(left.x * left.x + left.y * left.y + left.z * left.z);

}


// These are methods: need to be defined outside export{} (moved them above)
// GG: needed to add these methods to the objects first
Vec Point::to_vec() const {
    return _same<Point, Vec>(*this);
}

Norm Vec::to_norm() const {
    // GG: Vec does not have a norm method to invoke: use norm function defined above
    // GG: used _scalar_divide template function (operator / or * return a Vec, I want Norm)

    float length = _norm<Vec, float>(*this);
    return _scalar_divide<Vec, float, Norm>(*this, length);
}


export {

    // Sums
    /// Point += Vec -> Point
    Point operator+= (Point& p, const Vec& v) {
        p = _sum<Point, Vec, Point>(p, v);
        return p;
    }

    Point operator+ (const Point& p, const Vec& v) {
        return _sum<Point, Vec, Point>(p, v);
    }

    // GG: Technically given the sum is commutative one could decide to sum Vec + Point, but I would
    // avoid it for logical coherence

    /// Vec += Vec -> Vec
    Vec operator+= (Vec& v, const Vec& other) {
        v = _sum<Vec, Vec, Vec>(v, other);
        return v;
    }

    /// Vec + Vec -> Vec
    Vec operator+ (const Vec& v, const Vec& other) {
        return _sum<Vec, Vec, Vec>(v, other);
    }

    // Differences

    /// Point - Vec -> Point
    Point operator-= (Point& p, const Vec& v) {
        p = _difference<Point, Vec, Point>(p, v);
        return p;
    }

    Point operator- (const Point& p, const Vec& v) {
        return _difference<Point, Vec, Point>(p, v);
    }

    /// Vec -= Vec -> Vec
    Vec operator-= (Vec& v, const Vec& other) {
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
    /// Point * scalar -> Point
    Point operator*= (Point& p, float scalar) {
        // GG: Need to assign the result to the calling object
        p = _scalar_multiply<Point, float, Point>(p, scalar);
        return p;
    }

    Point operator* (const Point& p, float scalar) {
        return _scalar_multiply<Point, float, Point>(p, scalar);
    }

    Point operator* (float scalar, const Point& p) {
        return _scalar_multiply<Point, float, Point>(p, scalar);
    }

    /// Vec * scalar -> Vec
    Vec operator*= (Vec& v, float scalar) {
        // GG: Need to assign the result to the calling object
        v = _scalar_multiply<Vec, float, Vec>(v, scalar);
        return v;
    }

    Vec operator* (const Vec& v, float scalar) {
        return _scalar_multiply<Vec, float, Vec>(v, scalar);
    }

    Vec operator* (float scalar, const Vec& v) {
        return _scalar_multiply<Vec, float, Vec>(v, scalar);
    }

    // Dot products

    // GG: It does not make sense to do the scalar product between a Point and a Norm (direction)
    // Dot products are used to find the projections of a direction (a Vec or Norm) on another or the angle
    // between them, so it does not make sense to try to make the dot product between a position and a direction
    //float operator* (const Point& p, const Norm& n) {
    //    auto res = _dot_product<Point, Norm, Vec>(p, n);
    //    return res.x + res.y + res.z;
    //}

    /// Dot product between two Vec
    float operator* (const Vec& v, const Vec& other) {
        auto res = _dot_product<Vec, Vec, Vec>(v, other);
        return res.x + res.y + res.z;
    }
    /// Dot products between a Vec and a Norm
    float operator* (const Vec& v, const Norm& n) {
        auto res = _dot_product<Vec, Norm, Vec>(v, n);
        return res.x + res.y + res.z;
    }
    /// Dot product between two Norm
    float operator* (const Norm& v, const Norm& n) {
        auto res = _dot_product<Norm, Norm, Vec>(v, n);
        return res.x + res.y + res.z;
    }

    // Norms

    // Conversion to string
    // Point, Vec, Norm
    template<typename Obj> std::string conv_to_string(const Obj &obj) {
        return std::format("{}({:.2f}, {:.2f}, {:.2f})",
                           _get_name(obj), obj.x, obj.y, obj.z);
    }

    // Matrix
    std::string conv_to_string(HomMatrix M) {

        return std::format(
            "HomMatrix(\n"
            "  [{:.2f}, {:.2f}, {:.2f}, {:.2f}]\n"
            "  [{:.2f}, {:.2f}, {:.2f}, {:.2f}]\n"
            "  [{:.2f}, {:.2f}, {:.2f}, {:.2f}]\n"
            "  [{:.2f}, {:.2f}, {:.2f}, {:.2f}]\n"
            ")",
            M.mat[0], M.mat[1], M.mat[2], M.mat[3],
            M.mat[4], M.mat[5], M.mat[6], M.mat[7],
            M.mat[8], M.mat[9], M.mat[10], M.mat[11],
            M.mat[12], M.mat[13], M.mat[14], M.mat[15]
        );

    }

};