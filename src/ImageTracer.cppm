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

//#include <format>

export module ImageTracer;

import auxiliary_functions;
import std;
import Geometry;
import Color;
import HDRImage;
import PCG;
import Shape;
import Camera;

// ================================================================
// IMAGE TRACER
// ================================================================

// ImageTracer fire_all_rays() utilizes procedural approach for passing algorithm for
// solution of ray tracing equation

// Discrete map of pixels, used to store the rendered image.
// GG: changed it. Pass an HDRImage as parameter
export struct ImageTracer {
    ImageTracer(HDRImage& frame, Camera& camera)
        : frame(frame), camera(camera) {}
    Camera& camera; // Reference to the camera used for ray generation
    HDRImage frame;

    // GG: Added default values to u_pixel and v_pixel to let the ray pass through the center of the pixel
    [[nodiscard]] Ray fire_ray(int row, int col, float u_pixel=0.5f, float v_pixel=0.5f) const; // Generate a ray from the camera through the pixel at pixel coordinates (row, col) with subpixel offsets (u_pixel, v_pixel)
    void fire_all_rays(const std::function<Color(const Ray&)>& func);
    void fire_all_rays(const std::function<Color(const Ray&)>& func, PCG& pcg, int antialiasing_level = 1);

    // Trying to improve performance: flat renderer without passing the functional
    void fire_all_rays_flat(PCG& pcg, const World& world, Color sky_color) {
        const int width = frame.width;
        const int height = frame.height;

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                Ray ray = fire_ray(row, col, 0.5f, 0.5f);
                auto hit = world.ray_intersection(ray);
                if (hit.has_value()) {
                    frame.set_pixel(col, row, hit->hitted_shape->material->brdf->pigment->get_color(hit->surface_params));
                } else {
                    frame.set_pixel(col, row, sky_color);
                }
            }
        }
    }
};

// RP: SUS
// GG: Leave the bug, Tomasi specifically required this in order to be able to see something
//     fail in the following of the course
//                             ___________________________________
//                            |                                   |
//                            |        ___________                |
//                            |       |           |               |
//                            |       |           |               |
//                            |       |           |               |
//                            |       |           |               |
//                            |       |           |               |
//                            |       |           |               |
Ray ImageTracer::fire_ray(const int row, const int col, const float u_pixel, const float v_pixel) const {
    const float width = static_cast<const float>(frame.width);
    const float height = static_cast<const float>(frame.height);

    const float u = (static_cast<const float>(col) + u_pixel) / width;
    const float v = 1.0f - (static_cast<const float>(row) + v_pixel) / height;

    return camera.fire_ray(u, v);
}

// FIRE ALL RAYS

void ImageTracer::fire_all_rays(const std::function<Color(const Ray&)>& func) {
    const int width = frame.width;
    const int height = frame.height;

    // Takes a function as an argument: it will be the algorithm to solve the rendering equation
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            Ray ray = fire_ray(row, col, 0.5f, 0.5f);
            Color color = func(ray);
            frame.set_pixel(col, row, color);
        }
    }
}

void ImageTracer::fire_all_rays(const std::function<Color(const Ray&)>& func, PCG& pcg, int antialiasing_level) {
    const int width = frame.width;
    const int height = frame.height;
    if (antialiasing_level < 1) antialiasing_level = 1;

    // Takes a function as an argument: it will be the algorithm to solve the rendering equation
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {

            Color sum; // Cumulates sampled Colors
            for (int iu = 0; iu < antialiasing_level; iu++) {
                for (int iv = 0; iv < antialiasing_level; iv++) {
                    float u = (float(iu) + pcg.random_float() ) / antialiasing_level;
                    float v = (float(iv) + pcg.random_float() ) / antialiasing_level;
                    Ray ray = fire_ray(row, col, u, v);
                    sum += func(ray);
                }
            }
            frame.set_pixel(col, row, sum / float(antialiasing_level * antialiasing_level));
        }
    }
}