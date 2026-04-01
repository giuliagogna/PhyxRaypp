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
    std::array<float, 16> mat = {1.0f, 0.0f, 0.0f, 0.0f,
                                 0.0f, 1.0f, 0.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 0.0f, 1.0f};
};

/// Conversion to string of objects Point, Vec, Norm

const char* get_name(const Point&) { return "Point"; }
const char* get_name(const Vec&)   { return "Vec"; }
const char* get_name(const Norm&)  { return "Norm"; }

export template<typename Obj> std::string conv_to_string(const Obj &obj) {
    return std::format("{}({:.2f}, {:.2f}, {:.2f})",
                       get_name(obj), obj.x, obj.y, obj.z);
}

export std::string conv_to_string(HomMatrix M) {

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

export template<typename L, typename R, typename Res> Res _sum (const L &left, const R &right) {
    return Res{
        left.x + right.x,
        left.y + right.y,
        left.z + right.z
    };
}

export Point operator + (const Point &left, const Vec &right) {
    return _sum<Point, Vec, Point>(left, right);
}



export template<typename L, typename R, typename Res> Res _difference (L left, R right) {
    return Res{
        left.x - right.x,
        left.y - right.y,
        left.z - right.z
    };
}

export template<typename L, typename R, typename Res> Res _negate (L left) {
    return Res{
        -left.x,
        -left.y,
        -left.z
    };
}

export template<typename L, typename R, typename Res> Res _dot_product (L left, R right) {
    return Res{
        left.x * right.x,
        left.y * right.y,
        left.z * right.z
    };
}

export template<typename L, typename R, typename Res> Res _cross_product (L left, R right) {
    return Res{
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

export template<typename L, typename R, typename Res> Res _scalar_multiply (L left, R scalar) {
    return Res{
        left.x * scalar,
        left.y * scalar,
        left.z * scalar
    };
}

export template<typename L, typename R, typename Res> Res _scalar_divide (L left, R scalar) {
    return Res{
        left.x / scalar,
        left.y / scalar,
        left.z / scalar
    };
}
