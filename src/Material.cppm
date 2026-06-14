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
 * @file Material.cppm
 * @brief Material definition used by the renderer.
 *
 * A material describes how a surface interacts with light through
 * a BRDF and an optional emitted radiance term.
 */

module;
export module Material;

import std;
import Color;
import Geometry;
import Pigment;
import BRDF;

/**
 * @brief Surface material.
 *
 * A material combines:
 *
 * - a BRDF, describing how incoming light is scattered
 * - an emitted radiance term, describing light emitted by the surface
 *
 * Emissive materials can therefore act as light sources.
 */
export struct Material {

    /// Surface scattering model.
    std::shared_ptr<BRDF> brdf;

    /// Radiance emitted by the surface.
    std::shared_ptr<Pigment> emitted_radiance;

    /**
     * @brief Construct a material from a BRDF and an emission term.
     *
     * By default, creates a white diffuse material with no emission.
     *
     * @param brdf: Surface scattering model.
     * @param emitted_radiance: Emitted radiance pigment.
     */
    Material(
        std::shared_ptr<BRDF> brdf = std::make_shared<DiffusiveBRDF>(
            std::make_shared<UniformPigment>(Color{1.0f, 1.0f, 1.0f})
        ),

        std::shared_ptr<Pigment> emitted_radiance = std::make_shared<UniformPigment>(Color{0.0f, 0.0f, 0.0f})

    ) : brdf(std::move(brdf)), emitted_radiance(std::move(emitted_radiance)) {}

};