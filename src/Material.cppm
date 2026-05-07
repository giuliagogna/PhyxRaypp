module;
export module Material;

import std;
import Color;
import Geometry;
import Pigment;
import BRDF;

// Struct conttaining the BRDF and emitted radiance
export struct Material {
    std::shared_ptr<BRDF> brdf;
    std::shared_ptr<Pigment> emitted_radiance;

    Material(std::shared_ptr<BRDF> brdf, std::shared_ptr<Pigment> emitted_radiance) : brdf(brdf), emitted_radiance(emitted_radiance) {}
};

