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
// TEST 1: String Conversions
// =============================================================================
TEST_CASE("Test 1: String conversions") {
    Point p(1.3f, 4.6f, -7.8f);
    Vec v(4.6f, -33.9f, 0.012f);
    Norm n(1.0f, 0.0f, 0.0f);
    HomMatrix M{{1.0f, 2.0f, 3.0f, 4.0f,
                 5.0f, 6.0f, 7.0f, 8.0f,
                 9.0f, 10.0f, 11.0f, 12.0f,
                 0.0f, 0.0f, 0.0f, 1.0f}};

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

// =============================================================================
// TEST 2: Sums (Operator + and +=)
// =============================================================================
TEST_CASE("Test 2: Sum Operations") {
    Point p1(1.0f, 2.0f, 3.0f);
    Vec v1(4.0f, 5.0f, 6.0f);
    Vec v2(7.0f, 8.0f, 9.0f);

    SUBCASE("Point += Vec") {
        Point p_mut = p1;
        p_mut += v1;
        CHECK(aux::are_xyz_close(p_mut, Point(5.0f, 7.0f, 9.0f)));
    }

    SUBCASE("Point + Vec -> Point") {
        Point res = p1 + v1;
        CHECK(aux::are_xyz_close(res, Point(5.0f, 7.0f, 9.0f)));
    }

    SUBCASE("Vec += Vec") {
        Vec v_mut = v1;
        v_mut += v2;
        CHECK(aux::are_xyz_close(v_mut, Vec(11.0f, 13.0f, 15.0f)));
    }

    SUBCASE("Vec + Vec -> Vec") {
        Vec res = v1 + v2;
        CHECK(aux::are_xyz_close(res, Vec(11.0f, 13.0f, 15.0f)));
    }
}

// =============================================================================
// TEST 3: Differences (Operator - and -=)
// =============================================================================
TEST_CASE("Test 3: Difference Operations") {
    Point p1(10.0f, 20.0f, 30.0f);
    Point p2(1.0f, 2.0f, 3.0f);
    Vec v1(4.0f, 5.0f, 6.0f);
    Vec v2(2.0f, 1.0f, 3.0f);

    SUBCASE("Point -= Vec") {
        Point p_mut = p1;
        p_mut -= v1;
        CHECK(aux::are_xyz_close(p_mut, Point(6.0f, 15.0f, 24.0f)));
    }

    SUBCASE("Point - Vec -> Point") {
        Point res = p1 - v1;
        CHECK(aux::are_xyz_close(res, Point(6.0f, 15.0f, 24.0f)));
    }

    SUBCASE("Vec -= Vec") {
        Vec v_mut = v1;
        v_mut -= v2;
        CHECK(aux::are_xyz_close(v_mut, Vec(2.0f, 4.0f, 3.0f)));
    }

    SUBCASE("Vec - Vec -> Vec") {
        Vec res = v1 - v2;
        CHECK(aux::are_xyz_close(res, Vec(2.0f, 4.0f, 3.0f)));
    }

    SUBCASE("Point - Point -> Vec") {
        Vec res = p1 - p2;
        CHECK(aux::are_xyz_close(res, Vec(9.0f, 18.0f, 27.0f)));
    }
}

// =============================================================================
// TEST 4: Negation (Unary Operator -)
// =============================================================================
TEST_CASE("Test 4: Negation Operations") {
    Point p(1.0f, -2.0f, 3.0f);
    Vec v(-4.0f, 5.0f, -6.0f);

    SUBCASE("Negate Vec") {
        Vec res = -v;
        CHECK(aux::are_xyz_close(res, Vec(4.0f, -5.0f, 6.0f)));
    }

    SUBCASE("Negate Point") {
        Point res = -p;
        CHECK(aux::are_xyz_close(res, Point(-1.0f, 2.0f, -3.0f)));
    }
}

// =============================================================================
// TEST 5: Scalar Multiplications
// =============================================================================
TEST_CASE("Test 5: Scalar Multiplications") {
    Point p(1.0f, 2.0f, 3.0f);
    Vec v(4.0f, 5.0f, 6.0f);
    float scalar = 2.0f;

    SUBCASE("Point *= scalar") {
        Point p_mut = p;
        p_mut *= scalar;
        CHECK(aux::are_xyz_close(p_mut, Point(2.0f, 4.0f, 6.0f)));
    }

    SUBCASE("Point * scalar") {
        Point res = p * scalar;
        CHECK(aux::are_xyz_close(res, Point(2.0f, 4.0f, 6.0f)));
    }

    SUBCASE("scalar * Point") {
        Point res = scalar * p;
        CHECK(aux::are_xyz_close(res, Point(2.0f, 4.0f, 6.0f)));
    }

    SUBCASE("Vec *= scalar") {
        Vec v_mut = v;
        v_mut *= scalar;
        CHECK(aux::are_xyz_close(v_mut, Vec(8.0f, 10.0f, 12.0f)));
    }

    SUBCASE("Vec * scalar") {
        Vec res = v * scalar;
        CHECK(aux::are_xyz_close(res, Vec(8.0f, 10.0f, 12.0f)));
    }

    SUBCASE("scalar * Vec") {
        Vec res = scalar * v;
        CHECK(aux::are_xyz_close(res, Vec(8.0f, 10.0f, 12.0f)));
    }
}

// =============================================================================
// TEST 6: Dot Products
// =============================================================================
TEST_CASE("Test 6: Dot Products") {
    Vec v1(4.0f, -5.0f, 6.0f);
    Vec v2(-1.0f, 2.0f, -3.0f);
    Norm n1(0.0f, 1.0f, 0.0f);
    Norm n2(1.0f, 0.0f, 0.0f);

    SUBCASE("Vec * Vec") {
        float res = v1 * v2; // (4 * -1) + (-5 * 2) + (6 * -3) = -4 - 10 - 18 = -32
        CHECK(aux::are_close(res, -32.0f));
    }

    SUBCASE("Vec * Norm") {
        float res = v1 * n1; // (4 * 0) + (-5 * 1) + (6 * 0) = -5
        CHECK(aux::are_close(res, -5.0f));
    }

    SUBCASE("Norm * Norm") {
        float res = n1 * n2; // Orthogonal normals -> 0
        CHECK(aux::are_close(res, 0.0f));
    }
}

// =============================================================================
// TEST 7: Object Conversions and Normalization
// =============================================================================
TEST_CASE("Test 7: Object Conversions and Normalization (to_vec, to_norm, normalize)") {
    SUBCASE("Point::to_vec") {
        Point p(1.0f, 2.0f, 3.0f);
        Vec v = p.to_vec();
        CHECK(aux::are_xyz_close(v, Vec(1.0f, 2.0f, 3.0f)));
    }

    SUBCASE("Vec::to_norm") {
        // A vector of length 5 (3-4-5 triangle on xy plane)
        Vec v(3.0f, 4.0f, 0.0f);
        Norm n = v.to_norm();

        // Expected normalized values: x=3/5=0.6, y=4/5=0.8, z=0
        CHECK(aux::are_xyz_close(n, Norm(0.6f, 0.8f, 0.0f)));
    }

    SUBCASE("Vec::normalize") {
        // A vector of length 5
        Vec v(3.0f, 4.0f, 0.0f);
        Vec normalized_v = v.normalize();

        // Must return a Vec (not a Norm) with length 1
        CHECK(aux::are_xyz_close(normalized_v, Vec(0.6f, 0.8f, 0.0f)));
    }
}

// =============================================================================
// TEST 8: Length and Squared Length (norm, norm2)
// =============================================================================
TEST_CASE("Test 8: Length and Squared Length") {
    SUBCASE("Vec::norm and Vec::norm2") {
        Vec v(3.0f, 4.0f, 5.0f); // Length should be 5

        CHECK(aux::are_close(v.norm(), 7.07106781f));
        CHECK(aux::are_close(v.norm2(), 50.0f));
    }

    SUBCASE("Norm::norm and Norm::norm2") {
        Norm n(0.6f, 0.8f, 0.0f); // Length should be exactly 1

        CHECK(aux::are_close(n.norm(), 1.0f));
        CHECK(aux::are_close(n.norm2(), 1.0f));
    }
}