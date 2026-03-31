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
export module Color;
import std;
import auxiliary_functions;

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


    // Using the native /=0 float check to avoid overhead.

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


    // Checks if two colors are close (uses are_close from auxiliary_functions)
    bool is_close(const Color& other, float epsilon = 1e-6f) const {
        return aux::are_close(r, other.r, epsilon) &&
               aux::are_close(g, other.g, epsilon) &&
               aux::are_close(b, other.b, epsilon);
    }

    // =========================================================================
    // LUMINOSITY FUNCTIONS
    // =========================================================================

    // Pixel luminosity
    // GG: overkill to implement three functions given we were given as best option the Shirley & Morley (2003)
    //     suggestion
    
    /// Mid-range luminosity: (max(r, g, b) + min(r, g, b)) / 2
    [[nodiscard]] float luminosity_mid_range() const {
        return (std::max({r, g, b}) + std::min({r, g, b})) / 2.0;
    }

    /// Arithmetic mean luminosity: (r + g + b) / 3
    [[nodiscard]] float luminosity_arithmetic_mean() const {
        return (r + g + b) / 3.0;
    }

    /// BT.709 luminosity: 0.2126 * r + 0.7152 * g + 0.0722 * b
    [[nodiscard]] float luminosity_bt709() const {
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }   

};


// Custom formatter for Color to enable std::format support.
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