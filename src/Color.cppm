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
 * @file Color.cppm
 * @brief RGB color representation and color arithmetic.
 *
 * This module defines the Color type used throughout the renderer together with common arithmetic
 * operations and luminosity metrics.
 */

module;
export module Color;
import std;
import auxiliary_functions;

/**
 * @brief RGB color.
 *
 * Colors are represented as three floating-point components corresponding to red, green, and blue channels.
 */
export struct Color {
    /// Red component.
    float r{0.f};
    /// Green component.
    float g{0.f};
    /// Blue component.
    float b {0.f};

    /// Add another color component-wise.
    constexpr Color& operator+=(const Color& other) {
        r += other.r;
        g += other.g;
        b += other.b;
        return *this;
    }
    /// Return the component-wise sum of two colors.
    friend constexpr Color operator+(Color lvalue, const Color& rvalue) {
        lvalue += rvalue;
        return lvalue;
    }

    /// Subtract another color component-wise.
    constexpr Color& operator-=(const Color& other) {
        r -= other.r;
        g -= other.g;
        b -= other.b;
        return *this;
    }
    /// Return the component-wise difference of two colors.
    friend constexpr Color operator-(Color lvalue, const Color& rvalue) {
        lvalue -= rvalue;
        return lvalue;
    }

    /// Multiply two colors component-wise.
    constexpr Color& operator*=(const Color& other) {
        r *= other.r;
        g *= other.g;
        b *= other.b;
        return *this;
    }
    /// Return the component-wise multiplication of two colors.
    friend constexpr Color operator*(Color lvalue, const Color& rvalue) {
        lvalue *= rvalue;
        return lvalue;
    }

    /// Scale all color channels by a scalar.
    constexpr Color& operator*=(const float scalar) {
        r *= scalar;
        g *= scalar;
        b *= scalar;
        return *this;
    }

    /// Return the all color channels scaled by a scalar.
    friend constexpr Color operator*(Color lvalue, const float scalar) {
        lvalue *= scalar;
        return lvalue;
    }
    /// Return the all color channels scaled by a scalar.
    friend constexpr Color operator*(const float scalar, Color rvalue) {
        rvalue *= scalar;
        return rvalue;
    }

    /// Divide color channels component-wise.
    constexpr Color& operator/=(const Color& other) {
        r /= other.r;
        g /= other.g;
        b /= other.b;
        return *this;
    }
    /// Return all color channels divided component-wise
    friend constexpr Color operator/(Color lvalue, const Color& rvalue) {
        lvalue /= rvalue;
        return lvalue;
    }

    /// Divide all color channels by a scalar.
    constexpr Color& operator/=(const float scalar) {
        //assert(scalar != 0.f && "Illegal scalar division")
        float inverse = 1.f / scalar;
        r *= inverse;
        g *= inverse;
        b *= inverse;
        return *this;
    }
    /// Return all color channels divided by a scalar.
    friend constexpr Color operator/(Color lvalue, const float scalar) {
        lvalue /= scalar;
        return lvalue;
    }

    /**
     * @brief Compare two colors within a tolerance.
     *
     * @param other Color to compare against.
     * @param epsilon Maximum allowed component-wise difference.
     *
     * @return True if all channels are sufficiently close.
     */
    bool is_close(const Color& other, float epsilon = 1e-6f) const {
        return aux::are_close(r, other.r, epsilon) &&
               aux::are_close(g, other.g, epsilon) &&
               aux::are_close(b, other.b, epsilon);
    }

    // =========================================================================
    // Luminosity estimators
    // =========================================================================
    
    /**
     * @brief Compute the mid-range luminosity.
     *
     * Defined as the average of the largest and smallest channel.
     */
    [[nodiscard]] float luminosity_mid_range() const {
        return (std::max({r, g, b}) + std::min({r, g, b})) / 2.0;
    }

    /**
     * @brief Compute luminosity as the arithmetic mean of the channels.
     */
    [[nodiscard]] float luminosity_arithmetic_mean() const {
        return (r + g + b) / 3.0;
    }

    /**
     * @brief Compute luminosity using BT.709 luminance weights.
     *
     * This estimator approximates perceived brightness by giving
     * greater importance to the green channel.
     */
    [[nodiscard]] float luminosity_bt709() const {
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }   

};


// ============================================================
// std::format support
// ============================================================

/// Enable std::format() support for Color.
export template <>
struct std::formatter<Color> {
    std::formatter<float> float_fmt;

    constexpr auto parse(std::format_parse_context& ctx) {
        return float_fmt.parse(ctx);
    }

    // libc++ requires a generic formatting context type here.
    auto format(const Color& c, auto& ctx) const {
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