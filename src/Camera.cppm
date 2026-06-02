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
import PCG;


// ================================================================
// RAY
// ================================================================

// Ray struct, used for light rays. Needs to be cache-optimized.
export struct Ray {
    Point origin{0.0f, 0.0f, 0.0f};
    Vec direction{0.0f, 0.0f, 0.0f};
    float tmin{1e-5f}; // Minimum t value for ray intersection (to avoid self-intersection)
    float tmax{std::numeric_limits<float>::infinity()};
    int depth{0}; // Depth of the ray (number of bounces)

    // Methods
    [[nodiscard]] bool is_close(const Ray& other_ray, float tolerance = 1e-5f) const {
        return (origin.is_close(other_ray.origin, tolerance) && direction.is_close(other_ray.direction, tolerance));
    };

    [[nodiscard]] Point at(float t) const {
        return origin + (direction * t);
    }

    // RP: I don't feel it's spontaneous to have it here.
    // GG: You mean implementation? I put it here because it's just a return, but I can move it
    Ray transform(const Transformation& trans) const;
};
// We will need to manage transformations
// GG: We have them in Geometry


// ================================================================
// CAMERA
// ================================================================
export struct Camera {
    float aspect_ratio; // Image aspect ratio (width/height)
    Transformation trans; // Camera transformation (position and orientation that will be applied to rays generated in camera space)

    // GG: Added the default transformation (identity). Now we can call the Camera only with aspect ratio
    Camera(float aspect_ratio=1.0f, const Transformation& trans = Transformation{})
            : aspect_ratio(aspect_ratio), trans(trans) {} //cannot perform aggregate initialization with virtual methods!

    [[nodiscard]] virtual Ray fire_ray(float u, float v) const = 0; // Generate a ray from the camera through the pixel at normalized (u, v)
    // (1,0)------------------(1,1)
    //   |                    |
    //   |                    |
    //   |                    |
    //   |                    |
    //   |                    |
    // (0,0)------------------(0,1)
    // always put a virtual destructor!
    virtual ~Camera() = default;
};

export struct OrthogonalCamera : Camera {
    OrthogonalCamera(float aspect_ratio=1.0f, const Transformation& trans = Transformation{})
        : Camera(aspect_ratio, trans) {}
    [[nodiscard]] Ray fire_ray(float u, float v) const override;
};

export struct PerspectiveCamera : Camera {
    float d; // Screen-observer distance
    PerspectiveCamera(float aspect_ratio=1.0f, float d=1.0f, const Transformation& trans=Transformation{})
        : Camera(aspect_ratio, trans), d(d) {}
    [[nodiscard]] Ray fire_ray(float u, float v) const override;
};

// ================================================================
// IMPLEMENTATION
// ================================================================


// RAY TRANSFORM
Ray Ray::transform(const Transformation& trans) const {
    return Ray{
        .origin = trans * origin,
        .direction = trans * direction,
        .tmin = tmin,
        .tmax = tmax,
        .depth = depth
    };
}

// FIRE RAY
// Declared this way, u and v are always between 0 and 1
Ray OrthogonalCamera::fire_ray(const float u, const float v) const {
    // Ray origin is on the image plane at distance d from the camera position
    const Point ray_origin{-1.0f, (1.0f - 2.0f * u) * aspect_ratio, 2.0f * v - 1.0f}; // Camera space origin
    constexpr Vec ray_direction{1.0f, 0.0f, 0.0f}; // Camera space direction (orthogonal to the image plane)
    return Ray{ray_origin, ray_direction}.transform(trans);
}

Ray PerspectiveCamera::fire_ray(const float u, const float v) const {
    // Ray origin is the camera position (0,0,0 in camera space)
    const Point ray_origin{-d, 0.0f, 0.0f}; // Camera space origin
    const Vec ray_direction{d, (1.0f - 2.0f * u) * aspect_ratio, 2.0f * v - 1.0f}; // Camera space direction (from the camera position to the pixel on the image plane)
    return Ray{ray_origin, ray_direction}.transform(trans);
}

