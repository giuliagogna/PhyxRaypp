module;
#include <assert.h>
export module Color;
import std;

export struct Color {
    float r{0.f}, g{0.f}, b {0.f};

    // Sum
    constexpr Color& operator+=(const Color& other) {
        r += other.r;
        g += other.g;
        b += other.b;
        return *this;
    }
    // Sum
    friend constexpr Color operator+(Color lvalue, const Color& rvalue) {
        lvalue += rvalue;
        return lvalue;
    }


    // Difference
    constexpr Color& operator-=(const Color& other) {
        r -= other.r;
        g -= other.g;
        b -= other.b;
        return *this;
    }

    // Difference
    friend constexpr Color operator-(Color lvalue, const Color& rvalue) {
        lvalue -= rvalue;
        return lvalue;
    }


    // Commutative and distributive because
    // It's commutative and ditributive component-wise.

    // Component-wise product
    constexpr Color& operator*=(const Color& other) {
        r *= other.r;
        g *= other.g;
        b *= other.b;
        return *this;
    }

    // Component-wise product
    friend constexpr Color operator*(Color lvalue, const Color& rvalue) {
        lvalue *= rvalue;
        return lvalue;
    }


    // Product with scalar (float)
    constexpr Color& operator*=(const float scalar) {
        r *= scalar;
        g *= scalar;
        b *= scalar;
        return *this;
    }

    // Product with scalar (float)
    friend constexpr Color operator*(Color lvalue, const float scalar) {
        lvalue *= scalar;
        return lvalue;
    }

    // Product with scalar (float)
    friend constexpr Color operator*(const float scalar, Color rvalue) {
        rvalue *= scalar;
        return rvalue;
    }


    // I'll probabliy leave the native /=0 float check to avoid overhead.
    // I'll probably never use it actually.

    // Division component-wise (not really interesting)
    constexpr Color& operator/=(const Color& other) {
        //assert(other.r != 0.f && other.g != 0.f && other.b != 0.f && "Illegal component-wise division")
        r /= other.r;
        g /= other.g;
        b /= other.b;
        return *this;
    }

    // Division component-wise (not really interesting)
    friend constexpr Color operator/(Color lvalue, const Color& rvalue) {
        lvalue /= rvalue;
        return lvalue;
    }


    // I'll probabliy leave the native /=0 float check to avoid overhead.

    // Division by scalar (float)
    constexpr Color& operator/=(const float scalar) {
        //assert(scalar != 0.f && "Illegal scalar division")
        float inverse = 1.f / scalar;
        r *= inverse;
        g *= inverse;
        b *= inverse;
        return *this;
    }

    // Division by scalar (float)
    friend constexpr Color operator/(Color lvalue, const float scalar) {
        lvalue /= scalar;
        return lvalue;
    }


    // Modern C++ formatting for I/O and other buffers
    auto format(const Color& c, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{:.3f}, {:.3f}, {:.3f}", c.r, c.g, c.b);
    }

};

export template <>
struct std::formatter<Color> {
    std::formatter<float> float_fmt;

    constexpr auto parse(std::format_parse_context& ctx) {
        return float_fmt.parse(ctx);
    }

    auto format(const Color& c, std::format_context& ctx) const {
        auto it = ctx.out();        
        it = float_fmt.format(c.r, ctx);
        it = std::format_to(it, " ");
        ctx.advance_to(it); 
        it = float_fmt.format(c.g, ctx);
        it = std::format_to(it, " ");
        ctx.advance_to(it);
        return float_fmt.format(c.b, ctx);
    }
};