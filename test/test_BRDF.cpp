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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import std;
import Color;
import Geometry;
import auxiliary_functions;
import Pigment;
import BRDF;

TEST_CASE("Test BRDF constructors") {

    Color reflectance{1.0f, 2.0f, 3.0f};
    UniformPigment white{Color{1.0f, 1.0f, 1.0f}};

    SUBCASE("DiffusiveBRDF") {
        DiffusiveBRDF diffusive(std::make_shared<UniformPigment>(white), reflectance);
        CHECK(diffusive.reflectance.is_close(reflectance));
        CHECK(diffusive.pigment->get_color(Vec2D{0.0f, 0.0f}).is_close(white.get_color(Vec2D{0.0f, 0.0f})));
    }

    SUBCASE("SpecularBRDF") {
        float sharpness = 10.;
        SpecularBRDF specular(std::make_shared<UniformPigment>(white), reflectance, sharpness);
    }
}

TEST_CASE("Test BRDF::eval() for DiffusiveBRDF and SpecularBRDF") {
    Color reflectance{1.0f, 10.0f, 100.0f};
    UniformPigment pink{Color{2.0f, 1.0f, 1.0f}};

    SUBCASE("DiffusiveBRDF") {
        DiffusiveBRDF diffusive(std::make_shared<UniformPigment>(pink), reflectance);
        CHECK(diffusive.eval(Normal{1.0f, 0.0f, 0.0f}, Vec{-1.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}, Vec2D{0.0f, 0.0f}).is_close(Color{2.0f, 10.0f, 100.0f} / std::numbers::pi_v<float>));
    }

    SUBCASE("SpecularBRDF") {
        SpecularBRDF specular(std::make_shared<UniformPigment>(pink), reflectance, 1e5f);
        CHECK(specular.eval(Normal{1.0f, 0.0f, 0.0f}, Vec{-1.0f, 0.0f, 0.0f}, Vec{1.0f, 0.0f, 0.0f}, Vec2D{0.0f, 0.0f}).is_close(Color{2.0f, 10.0f, 100.0f}));

        std::println("Outcoming color: {}", specular.eval(Normal{1.0f, 0.0f, 0.0f}, Vec{-1.0f, 0.0f, 0.0f}, Vec{0.0f, 1.0f, 0.0f}, Vec2D{0.0f, 0.0f}));
        CHECK(specular.eval(Normal{1.0f, 0.0f, 0.0f}, Vec{-1.0f, 0.0f, 0.0f}, Vec{0.0f, 1.0f, 0.0f}, Vec2D{0.0f, 0.0f}).is_close(Color{0.0f, 0.0f, 0.0f}));
    }
}

