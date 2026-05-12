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

export module Renderer;

import std;
import Pigment;
import Color;
import HDRImage;
import Camera;
import Geometry;
import Shape;


// =========================================================================
// BASE RENDERER
// =========================================================================
export struct Renderer {
    const World* world;
    Color background_color;

    Renderer(const World* world, Color bg_color = Color{0.0f, 0.0f, 0.0f}) : world{world}, background_color{bg_color} {}

    virtual ~Renderer() = default;

    // The call operator allows the struct to be used exactly like a function/lambda
    [[nodiscard]] virtual Color operator()(const Ray& ray) const = 0;
};

// =========================================================================
// ON/OFF RENDERER (Black and White)
// =========================================================================
export struct OnOffRenderer : Renderer {
    Color hit_color;

    OnOffRenderer(const World* world, Color bg_color = Color{0.0f, 0.0f, 0.0f}, Color hit_color = Color{1.0f, 1.0f, 1.0f})
        : Renderer(world, bg_color), hit_color{hit_color} {}

    [[nodiscard]] Color operator()(const Ray& ray) const override {
        if (world->ray_intersection(ray).has_value()) {
            return hit_color;
        } else {
            return background_color;
        }
    }
};

// =========================================================================
// FLAT RENDERER (Pigment colors)
// =========================================================================
export struct FlatRenderer : Renderer {
    // Inherit constructors from Renderer
    using Renderer::Renderer;

    [[nodiscard]] Color operator()(const Ray& ray) const override {
        auto hit = world->ray_intersection(ray);

        if (hit.has_value()) {
            const Shape* shape = hit->hitted_shape;

            // Safety check: ensure the shape has a material and a BRDF attached
            if (shape->material && shape->material->brdf) {
                return shape->material->brdf->pigment->get_color(hit->surface_params);
            }

            // Fallback color (Red) if the shape has no material attached
            return Color{1.0f, 0.0f, 0.0f};
        }

        return background_color;
    }
};