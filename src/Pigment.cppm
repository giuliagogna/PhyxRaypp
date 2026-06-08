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
 * @file Pigment.cppm
 * @brief Surface color sources used by materials.
 *
 * This module defines pigments, which map surface coordinates
 * to colors and are used by materials and BRDFs.
 */

module;

export module Pigment;
import std;
import Color;
import Geometry;
import HDRImage;

/**
 * @brief Base class for surface pigments.
 *
 * A pigment defines how color varies across a surface as a function
 * of its parametric coordinates (u,v).
 *
 * Pigments can be constant, procedural, or image-based.
 */
export struct Pigment {
    /// Virtual destructor
    virtual ~Pigment() = default;

    /**
     * @brief Evaluate the pigment at a surface location.
     *
     * @param surface_params Surface parametric coordinates (u,v).
     * @return Color associated with the specified coordinates.
     */
    [[nodiscard]] virtual Color get_color(const Vec2D& surface_params) const = 0;
};

/**
 * @brief Pigment with a constant color.
 *
 * Returns the same color regardless of surface coordinates.
 */
export struct UniformPigment : Pigment {

    /// Constant pigment color.
    Color color;

    /// Construct a uniform pigment.
    UniformPigment(const Color& c) : color{c} {};

    [[nodiscard]] Color get_color(const Vec2D& surface_params) const override {
        return color;     // Ignore surface coordinates: the color is constant.
    }
};

/**
 * @brief Checkerboard pigment.
 *
 * Alternates two colors over a regular grid in texture space.
 */
export struct CheckeredPigment : Pigment {

    /// First checkerboard color.
    Color color1;

    /// Second checkerboard color.
    Color color2;

    /// Number of subdivisions along each texture axis.
    int num_steps;

    /**
     * @brief Construct a checkerboard pigment.
     *
     * @param color1 First color.
     * @param color2 Second color.
     * @param num_steps Number of grid subdivisions.
     */
    CheckeredPigment(const Color& color1, const Color& color2, int num_steps) : color1{color1}, color2{color2}, num_steps {num_steps} {};

    [[nodiscard]] Color get_color(const Vec2D& surface_params) const override {

        // Determine the checkerboard cell containing (u,v).
        int row = static_cast<int>(std::floor(surface_params.u * num_steps));
        int col = static_cast<int>(std::floor(surface_params.v * num_steps));

        // Alternate colors between neighboring cells.
        if (std::abs(row%2) == std::abs(col%2)) {
            return color1;
        } else {
            return color2;
        }
    }
};

/**
 * @brief Image-based pigment.
 *
 * Samples colors from an image using surface coordinates.
 */
export struct ImagePigment : Pigment {

    /// Texture image
    HDRImage image;

    /// Construct a pigment from an image texture.
    ImagePigment(const HDRImage& image) : image{image} {};

    [[nodiscard]] Color get_color(const Vec2D& surface_params) const override {

        // Convert normalized texture coordinates to image pixels.
        int col = static_cast<int>(surface_params.u * image.width);
        int row = static_cast<int>(surface_params.v * image.height);

        // Clamp coordinates to the image boundaries.
        col = std::clamp(col, 0, image.width - 1);
        row = std::clamp(row, 0, image.height - 1);

        return image.get_pixel(col, row);
    }
};