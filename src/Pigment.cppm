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

export module Pigment;
import std;
import Color;
import Geometry;
import HDRImage;

export struct Pigment {
    virtual ~Pigment() = default;
    [[nodiscard]] virtual Color get_color(const Vec2D& surface_params) const = 0;
};

export struct UniformPigment : Pigment {
    Color color;

    UniformPigment(const Color& c) : color{c} {};

    [[nodiscard]] Color get_color(const Vec2D& surface_params) const override {
        return color; // ignore the (u, v) coordinates, color is the same everywhere
    }
};

export struct CheckeredPigment : Pigment {
    Color color1;
    Color color2;
    int num_steps; // Defines number of subdivisions in the grid

    CheckeredPigment(const Color& color1, const Color& color2, int num_steps) : color1{color1}, color2{color2}, num_steps {num_steps} {};

    [[nodiscard]] Color get_color(const Vec2D& surface_params) const override {
        // Scale the (u, v) coordinates to find the coordinates in the grid cell
        int row = static_cast<int>(std::floor(surface_params.u * num_steps));
        int col = static_cast<int>(std::floor(surface_params.v * num_steps));

        // If row and col are both even or odd give a color, otherwise give the other
        if (std::abs(row%2) == std::abs(col%2)) {
            return color1;
        } else {
            return color2;
        }
    }
};

export struct ImagePigment : Pigment {
    HDRImage image;

    ImagePigment(const HDRImage& image) : image{image} {};

    [[nodiscard]] Color get_color(const Vec2D& surface_params) const override {
        int col = static_cast<int>(surface_params.u * image.width);
        int row = static_cast<int>(surface_params.v * image.height);

        col = std::clamp(col, 0, image.width - 1);
        row = std::clamp(row, 0, image.height - 1);

        return image.get_pixel(col, row);
    }
};