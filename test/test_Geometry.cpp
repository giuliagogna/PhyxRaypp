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
import auxiliary_functions;
import Geometry;

// =============================================================================
// Testing conversion to string
// =============================================================================

TEST_CASE("Testing conversion to string") {
    Point p(1.3, 4.6, -7.8);
    Vec v(4.6, -33.9, 0.012);
    Norm n(1.0, 0.0, 0.0);
    HomMatrix M{{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 0.0f, 0.0f, 0.0f, 1.0f}};

    SUBCASE("Conversion of Point, Vec, Norm") {
        CHECK(conv_to_string(p) == "Point(1.30, 4.60, -7.80)");
        CHECK(conv_to_string(v) == "Vec(4.60, -33.90, 0.01)"); // truncated at second decimal
        CHECK(conv_to_string(n) == "Norm(1.00, 0.00, 0.00)");
    }

    SUBCASE("Conversion of HomMatrix") {
        std::string expected_matrix =
            "HomMatrix(\n"
            "  [1.00, 2.00, 3.00, 4.00]\n"
            "  [5.00, 6.00, 7.00, 8.00]\n"
            "  [9.00, 10.00, 11.00, 12.00]\n"
            "  [0.00, 0.00, 0.00, 1.00]\n"
            ")";

        CHECK(conv_to_string(M) == expected_matrix);
    }
}