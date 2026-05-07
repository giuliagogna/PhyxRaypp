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

export struct Pigment {
    virtual ~Pigment() = default;
    [[nodiscard]] virtual Color get_color(const Vec2D& uv) const = 0;
};

export struct UniformPigment : Pigment {
    Color color;

    UniformPigment(const Color& c) : color{c} {}

    [[nodiscard]] Color get_color(const Vec2D& uv) const override {
        return color; // Ignora le UV, il colore è uguale ovunque
    }
};