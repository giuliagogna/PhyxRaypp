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

export module BRDF;
import std;
import Color;
import Geometry;
import Camera;
import Pigment;

// Virtual DRDF class
export struct BRDF {
    std::shared_ptr<Pigment> pigment;
    Color reflectance;

    // RP: Should I pass Color by referenc??? I'll test if I have time to...
    BRDF(std::shared_ptr<Pigment> pigment, Color reflectance) : pigment(pigment), reflectance(reflectance) {}
    virtual Color eval(Normal normal, Vec in_dir, Vec out_dir, Vec2D uv) const = 0;
};

// Pure isotropic diffusion BRDF
export struct DiffusiveBRDF : BRDF {
    DiffusiveBRDF(std::shared_ptr<Pigment> pigment, Color reflectance) : BRDF(pigment, reflectance) {}

    Color eval(Normal normal, Vec in_dir, Vec out_dir, Vec2D uv) const override {
        return pigment->get_color(uv) * (reflectance / std::numbers::pi_v<float>);
    }
};

// Specular reflection BRDF, chosen sharpness
export struct SpecularBRDF : BRDF {
    float sharpness;

    SpecularBRDF(std::shared_ptr<Pigment> pigment, Color reflectance, float sharpness = 10.0f) : BRDF(pigment, reflectance), sharpness(sharpness) {}
    Color eval(Normal normal, Vec in_dir, Vec out_dir, Vec2D uv) const override {
        
        float overlap = (out_dir.normalize() - in_dir.normalize()).to_norm() * normal;
        if (overlap > 1.0f) overlap = 1.0f;
        if (overlap < 0.0f) overlap = 0.0f;
        return pigment->get_color(uv) * reflectance * std::pow(overlap, sharpness);
    }
};


