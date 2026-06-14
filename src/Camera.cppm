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
 * @file Camera.cppm
 * @brief Ray, camera and image tracing utilities.
 *
 * This module defines the ray representation used throughout the renderer, camera models for ray generation,
 * and the ImageTracer responsible for converting image pixels into rays.
 */

module;

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

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

/**
 * @brief Ray used for visibility and light transport queries.
 *
 * A ray is defined by an origin, a direction, and a valid parameter interval [tmin, tmax].
 */
export struct Ray {
    /// Ray origin.
    Point origin{0.0f, 0.0f, 0.0f};

    /// Ray direction.
    Vec direction{0.0f, 0.0f, 0.0f};

    /// Minimum valid ray parameter.
    float tmin{1e-5f};

    /// Maximum valid ray parameter.
    float tmax{std::numeric_limits<float>::infinity()};

    /// Number of bounces undergone by the ray.
    int depth{0};


    // Methods

    /// Compare two rays within a tolerance.
    [[nodiscard]] bool is_close(const Ray& other_ray, float tolerance = 1e-5f) const {
        return (origin.is_close(other_ray.origin, tolerance) && direction.is_close(other_ray.direction, tolerance));
    };

    /**
     * @brief Evaluate a point along the ray.
     *
     * Computes:
     * origin + t * direction
     *
     * @param t Ray parameter.
     * @return Point located at parameter t.
     */
    [[nodiscard]] Point at(float t) const {
        return origin + (direction * t);
    }

    /// Apply a geometric transformation to the ray.
    Ray transform(const Transformation& trans) const {
        return Ray{
            .origin = trans * origin,
            .direction = trans * direction,
            .tmin = tmin,
            .tmax = tmax,
            .depth = depth
        };
    }
};


// ================================================================
// CAMERA
// ================================================================

/**
 * @brief Base class for camera models.
 *
 * Cameras generate rays passing through points on the image plane.
 * The generated rays are expressed in camera space and then transformed
 * into world space through the associated transformation.
 */
export struct Camera {
    /// Image aspect ratio (width / height).
    float aspect_ratio;

    /// Camera-to-world transformation.
    Transformation trans;

    /**
     * @brief Construct a camera.
     *
     * @param aspect_ratio Image aspect ratio.
     * @param trans Optional camera-to-world transformation.
     *              Defaults to the identity transformation.
     */
    Camera(float aspect_ratio=1.0f, const Transformation& trans = Transformation{})
            : aspect_ratio(aspect_ratio), trans(trans) {}

    /**
     * @brief Generate a ray through a normalized image-plane position.
     *
     * The coordinates (u,v) lie in the unit square [0,1] × [0,1].
     *
     * @param u Horizontal image coordinate.
     * @param v Vertical image coordinate.
     *
     * @return Generated ray.
     */
    [[nodiscard]] virtual Ray fire_ray(float u, float v) const = 0; // Generate a ray from the camera through the pixel at normalized (u, v)
    // (1,0)------------------(1,1)
    //   |                    |
    //   |                    |
    //   |                    |
    //   |                    |
    //   |                    |
    // (0,0)------------------(0,1)

    /// Virtual destructor
    virtual ~Camera() = default;
};

/**
 * @brief Orthographic camera.
 *
 * Generates parallel rays orthogonal to the image plane.
 */
export struct OrthogonalCamera : Camera {

    /**
     * @brief Construct an orthogonal camera.
     *
     * @param aspect_ratio Image aspect ratio.
     * @param trans Camera-to-world transformation. Defaults to identity.
     */
    OrthogonalCamera(float aspect_ratio=1.0f, const Transformation& trans = Transformation{})
        : Camera(aspect_ratio, trans) {}
    [[nodiscard]] Ray fire_ray(float u, float v) const override {
        const Point ray_origin{-1.0f, (1.0f - 2.0f * u) * aspect_ratio, 2.0f * v - 1.0f};
        constexpr Vec ray_direction{1.0f, 0.0f, 0.0f};
        return Ray{ray_origin, ray_direction}.transform(trans);
    }
};

/**
 * @brief Perspective camera.
 *
 * Generates rays originating from a single viewpoint and passing
 * through the image plane.
 */
export struct PerspectiveCamera : Camera {

    /// Distance between the viewpoint and the image plane.
    float d;

    /**
     * @brief Construct a perspective camera.
     *
     * @param aspect_ratio Image aspect ratio.
     * @param d Distance between the viewpoint and the image plane.
     * @param trans Camera-to-world transformation. Defaults to identity.
     */
    PerspectiveCamera(float aspect_ratio=1.0f, float d=1.0f, const Transformation& trans=Transformation{})
        : Camera(aspect_ratio, trans), d(d) {}
    [[nodiscard]] Ray fire_ray(float u, float v) const override {
        const Point ray_origin{-d, 0.0f, 0.0f};
        const Vec ray_direction{d, (1.0f - 2.0f * u) * aspect_ratio, 2.0f * v - 1.0f};
        return Ray{ray_origin, ray_direction}.transform(trans);
    };
};


// ================================================================
// IMAGE TRACER
// ================================================================

/**
 * @brief Converts image pixels into camera rays.
 *
 * The ImageTracer maps pixel coordinates to image-plane coordinates,
 * generates the corresponding camera rays, and stores the computed
 * colors in the output image.
 */
export struct ImageTracer {
    ImageTracer(HDRImage& frame, Camera& camera)
        : frame(frame), camera(camera) {}

    /// Camera used to generate rays.
    Camera& camera;

    /// Output image.
    HDRImage frame;

    /**
     * @brief Generate the ray associated with a pixel.
     *
     * Subpixel offsets can be specified for anti-aliasing and stochastic
     * sampling.
     *
     * @param row Pixel row.
     * @param col Pixel column.
     * @param u_pixel Horizontal subpixel offset.
     * @param v_pixel Vertical subpixel offset.
     *
     * @return Generated ray.
     */
    [[nodiscard]] Ray fire_ray(int row, int col, float u_pixel=0.5f, float v_pixel=0.5f) const {
        const float width = static_cast<const float>(frame.width);
        const float height = static_cast<const float>(frame.height);

        const float u = (static_cast<const float>(col) + u_pixel) / width;
        const float v = 1.0f - (static_cast<const float>(row) + v_pixel) / height;

        return camera.fire_ray(u, v);
    }

    /**
     * @brief Render the image using one sample per pixel.
     *
     * The supplied function evaluates the color associated with each ray.
     */
    void fire_all_rays(const std::function<Color(const Ray&)>& func) {
        const int width = frame.width;
        const int height = frame.height;

        // Render using a single sample per pixel.
        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                Ray ray = fire_ray(row, col, 0.5f, 0.5f);
                Color color = func(ray);
                frame.set_pixel(col, row, color);
            }
        }
    }

    /**
     * @brief Render the image using stratified supersampling.
     *
     * Multiple rays are generated per pixel and their contributions
     * are averaged to reduce aliasing artifacts (e.g. Moiré fringes).
     *
     * @param func Rendering function.
     * @param pcg Random number generator.
     * @param antialiasing_level Number of subdivisions per pixel axis.
     */
    void fire_all_rays(const std::function<Color(const Ray&)>& func, PCG& pcg, int antialiasing_level = 1) {
        const int width = frame.width;
        const int height = frame.height;
        if (antialiasing_level < 1) antialiasing_level = 1;

        const int total_samples = antialiasing_level * antialiasing_level;
        const float antialiasing_level_reciprocal = 1.0f / float(total_samples);
    
        // Cycle on pixels, and internally at each pixel cycle on subpixels to perform antialiasing
        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                Color sum; // Cumulates sampled Colors
                for (int i = 0; i < total_samples; i++) {
                    // Inter-pixel indexing
                    int iu = i / antialiasing_level;
                    int iv = i % antialiasing_level;
                    float u = (float(iu) + pcg.random_float()) / antialiasing_level;
                    float v = (float(iv) + pcg.random_float()) / antialiasing_level;
                    Ray ray = fire_ray(row, col, u, v);
                    sum += func(ray);
                }
                frame.set_pixel(col, row, sum * antialiasing_level_reciprocal); // Takes the average for that pixel
            }
        }
    }

    /**
     * @brief Render the image using one sample per pixel, accelerated by TBB parallelization.
     *
     * The supplied function evaluates the color associated with each ray.
     */
    void fire_all_rays_parallel(const std::function<Color(const Ray&)>& func) {
        const int width = frame.width;
        const int height = frame.height;

        // Parallelization over image rows
        tbb::parallel_for(tbb::blocked_range<int>(0, height), [&](const tbb::blocked_range<int>& range) {
            for (int row = range.begin(); row < range.end(); ++row) {
                for (int col = 0; col < width; ++col) {
                    // Fire a single ray through the center of the pixel
                    Ray ray = fire_ray(row, col, 0.5f, 0.5f);
                    Color color = func(ray);
                    // Update the pixel
                    frame.set_pixel(col, row, color);
                }
            }
        });
    }


    /**
     * @brief Render the image using stratified supersampling, accelerated by TBB parallelizazion.
     *
     * Multiple rays are generated per pixel and their contributions
     * are averaged to reduce aliasing artifacts (e.g. Moiré fringes).
     *
     * @param func Rendering function.
     * @param pcg Random number generator.
     * @param antialiasing_level Number of subdivisions per pixel axis.
     */
    void fire_all_rays_parallel(const std::function<Color(const Ray&)>& func, PCG& pcg, int antialiasing_level = 1) {
        const int width = frame.width;
        const int height = frame.height;
        if (antialiasing_level < 1) antialiasing_level = 1;

        const int total_samples = antialiasing_level * antialiasing_level;
        const float antialiasing_level_reciprocal = 1.0f / float(total_samples);

        // Parallelization over the image rows
        tbb::parallel_for(tbb::blocked_range<int>(0, height), [&](const tbb::blocked_range<int>& range) {
            PCG local_pcg(range.begin() * width + 1, range.begin() * width + 2); // Tricky enough, still deterministic
            for (int row = range.begin(); row < range.end(); ++row) {
                for (int col = 0; col < width; ++col) {

                    Color pixel_color_sum{0.0f, 0.0f, 0.0f};

                    for (int i = 0; i < total_samples; ++i) {
                        // Inter-pixel sub-grid coordinates
                        int iu = i / antialiasing_level;
                        int iv = i % antialiasing_level;

                        // Pixel relative coordinates
                        float u = (float(iu) + local_pcg.random_float()) / antialiasing_level;
                        float v = (float(iv) + local_pcg.random_float()) / antialiasing_level;

                        Ray ray = fire_ray(row, col, u, v);
                        pixel_color_sum += func(ray);
                    }
                    // Update the pixel
                    frame.set_pixel(col, row, pixel_color_sum * antialiasing_level_reciprocal);
                }
            }
        });
    }
};

/*
// ================================================================
// IMPLEMENTATION
// ================================================================

// Convert pixel coordinates to normalized image-plane coordinates.
Ray ImageTracer::fire_ray(const int row, const int col, const float u_pixel, const float v_pixel) const 


// FIRE ALL RAYS

void ImageTracer::fire_all_rays(const std::function<Color(const Ray&)>& func) 

void ImageTracer::fire_all_rays(const std::function<Color(const Ray&)>& func, PCG& pcg, int antialiasing_level) 

    // Renders the image without antialiasing, parallelized across rows using Intel TBB.
    // Fires a single ray through the center of each pixel.
    void fire_all_rays_parallel(const std::function<Color(const Ray&)>& func) {
        const int width = frame.width;
        const int height = frame.height;

        // Parallelization over image rows
        tbb::parallel_for(tbb::blocked_range<int>(0, height), [&](const tbb::blocked_range<int>& range) {
            for (int row = range.begin(); row < range.end(); ++row) {
                for (int col = 0; col < width; ++col) {
                    // Fire a single ray through the center of the pixel
                    Ray ray = fire_ray(row, col, 0.5f, 0.5f);
                    Color color = func(ray);
                    // Update the pixel
                    frame.set_pixel(col, row, color);
                }
            }
        });
    }


    // Renders the image with antialiasing, parallelized across rows using Intel TBB.
    void fire_all_rays_parallel(const std::function<Color(const Ray&)>& func, PCG& pcg, int antialiasing_level = 1) {
        const int width = frame.width;
        const int height = frame.height;
        if (antialiasing_level < 1) antialiasing_level = 1;

        const int total_samples = antialiasing_level * antialiasing_level;
        const float antialiasing_level_reciprocal = 1.0f / float(total_samples);

        // Parallelization over the image rows
        tbb::parallel_for(tbb::blocked_range<int>(0, height), [&](const tbb::blocked_range<int>& range) {
            PCG local_pcg(range.begin() * width + 1, range.begin() * width + 2); // Tricky enough, still deterministic
            for (int row = range.begin(); row < range.end(); ++row) {
                for (int col = 0; col < width; ++col) {

                    Color pixel_color_sum{0.0f, 0.0f, 0.0f};

                    for (int i = 0; i < total_samples; ++i) {
                        // Inter-pixel sub-grid coordinates
                        int iu = i / antialiasing_level;
                        int iv = i % antialiasing_level;

                        // Pixel relative coordinates
                        float u = (float(iu) + local_pcg.random_float()) / antialiasing_level;
                        float v = (float(iv) + local_pcg.random_float()) / antialiasing_level;

                        Ray ray = fire_ray(row, col, u, v);
                        pixel_color_sum += func(ray);
                    }
                    // Update the pixel
                    frame.set_pixel(col, row, pixel_color_sum * antialiasing_level_reciprocal);
                }
            }
        });
    }
};
*/