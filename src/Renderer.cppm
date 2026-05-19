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
import PCG;
import Material;


// =========================================================================
// BASE RENDERER
// =========================================================================
export struct Renderer {
    const World* world;
    Color background_color; // Emissive color of the environment (if ray does not hit anything)

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

// =========================================================================
// PATH TRACER (solution of rendering equation)
// =========================================================================

export struct PathTracer : Renderer {

    PCG& pcg;
    int num_of_rays;
    int max_depth;
    int russian_roulette_limit;

    using Renderer::Renderer;

    PathTracer(
        PCG& pcg,
        World* world,
        Color background_color = Color{0.0f, 0.0f, 0.0f},
        int num_of_rays = 10,
        int max_depth = 10,
        int russian_roulette_limit = 3
        ) : Renderer(world, background_color), pcg(pcg), num_of_rays(num_of_rays), max_depth(max_depth), russian_roulette_limit(russian_roulette_limit) {}

    [[nodiscard]] Color operator()(const Ray& ray) const override {

        // If the ray has been reflected too many times return black
        // This happens at the end of recursion if it's not truncated by the Russian Roulette before
        if(ray.depth > max_depth) {
            return Color{0.0f, 0.0f, 0.0f};
        }

        // If the ray does not intersect anything return the background (emissive) color and stops the recursion
        auto hit_record = world->ray_intersection(ray);
        if (!hit_record.has_value()) {
            return background_color;
        }

        // If the ray intersects something, we need to solve the rendering equation at the point of intersection
        auto hit_material = hit_record->hitted_shape->material;
        Color hit_color = hit_material->brdf->pigment->get_color(hit_record->surface_params); // the Color it "reflects"
        Color emitted_radiance = hit_material->emitted_radiance->get_color(hit_record->surface_params); // the Color it emits

        //////////////////////////////////////////////////////////////////////////////////////////////////
        // Russian Roulette: triggered only if the ray has been reflected at least russian_roulette_limit
        //////////////////////////////////////////////////////////////////////////////////////////////////
        // The probability of terminating the recursion is based on the luminosity of the current hit
        // "Load the bullets!"
        float hit_color_lum = std::max({hit_color.r, hit_color.g, hit_color.b});
        // "Pull the trigger!"
        if (ray.depth > russian_roulette_limit) { // First reflections are mandatory
            float q = std::max(0.05f, 1.0f-hit_color_lum);
            if (pcg.random_float() > q) {
                // Keep the recursion going, but compensate for other potentially discarded rays
                hit_color *= 1.0f/(1.0f - q);
            } else {
                // Stop the recursion prematurely 
                return emitted_radiance;
            }
        }

        // If the ray survived the Russian Roulette, it's time for the recursive call...

        // Montecarlo integration
        Color cum_radiance = Color{0.0f, 0.0f, 0.0f};

        if (hit_color_lum > 0.0f) { // Only do costly recursions if it's worth it
            for (int ray_index = 0; ray_index < num_of_rays; ray_index++) {

                Ray new_ray = hit_material->brdf->scatter_ray(
                    pcg,
                    hit_record->ray.direction,
                    hit_record->hit_point,
                    hit_record->hit_normal,
                    hit_record->ray.depth + 1
                );

                Color new_radiance = (*this)(new_ray);
                cum_radiance += hit_color * new_radiance; // Add the radiance of the new ray, obtained by recursion, to the current radiance
            }
        }

        return emitted_radiance + cum_radiance * 1.0f / num_of_rays;
    }
};