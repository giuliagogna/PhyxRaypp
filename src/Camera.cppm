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

export module Camera;

import auxiliary_functions;
import std;
import Geometry;
import Color;
import HDRImage;

// Ray struct, used for light rays. Needs to be cache-optimized.
export struct Ray {
    Point origin;
    Vec direction;
    float tmin{1e-5f}; // Minimum t value for ray intersection (to avoid self-intersection)
    float tmax{std::numeric_limits<float>::infinity()};
    int depth{0}; // Depth of the ray (number of bounces)

    // Methods
    bool is_close(const Ray& other_ray, float tolerance = 1e-5f) const {
        return (origin.is_close(other_ray.origin, tolerance) && direction.is_close(other_ray.direction, tolerance));
    };

    Point at(float t) const {
        return origin + (direction * t);
    }

    Ray transform(const Transformation& trans){
        return Ray(
                    .origin = trans * origin,
                    .direction = trans * direction,
                    .tmin = tmin,      
                    .tmax = tmax,      
                    .depth = depth     
                );
    }
};
// We will need to manage trasformations

export struct Camera {

    // The only adjustable parameters of Camera are d (screen-observer distance) and a (image aspect ratio).

    // here is possible that it just works with less parameters IDK

    
    Ray fire_ray(float u, float v) const; // Generate a ray from the camera through the pixel at normalized (u, v)
    // (0,0)------------------(1,0)
    //   |                    |
    //   |                    |
    //   |                    |
    //   |                    |
    //   |                    |
    // (0,0)------------------(1,0)
};

// Procedural!

// Discrete map of pixels, used to store the rendered image.
export struct ImageTracer
{
    Camera camera;
    int width;
    int height;

    HDRImage framebuffer;
    Ray fire_ray(int row, int col, float u_pixel, float v_pixel) const; // Generate a ray from the camera through the pixel at pixel coordinates (row, col) with subpixel offsets (u_pixel, v_pixel)
    void fire_rays();
};