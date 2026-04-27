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
// TEST 1: Sums (Operator + and +=)
// =============================================================================
TEST_CASE("Test 1: Sum Operations") {
    Point p1(1.0f, 2.0f, 3.0f);
    Vec v1(4.0f, 5.0f, 6.0f);
    Vec v2(7.0f, 8.0f, 9.0f);

    SUBCASE("Point += Vec -> Point") {
        Point p_mut = p1;
        p_mut += v1;

        CHECK(p_mut.is_close(Point(5.0f, 7.0f, 9.0f)));
    }

    SUBCASE("Point + Vec -> Point") {
        Point res = p1 + v1;
        CHECK(res.is_close(Point(5.0f, 7.0f, 9.0f)));
    }

    SUBCASE("Vec += Vec -> Vec") {
        Vec v_mut = v1;
        v_mut += v2;
        CHECK(v_mut.is_close(Vec(11.0f, 13.0f, 15.0f)));
    }

    SUBCASE("Vec + Vec -> Vec") {
        Vec res = v1 + v2;
        CHECK(res.is_close(Vec(11.0f, 13.0f, 15.0f)));
    }
}

// =============================================================================
// TEST 2: Differences (Operator - and -=)
// =============================================================================
TEST_CASE("Test 2: Difference Operations") {
    Point p1(10.0f, 20.0f, 30.0f);
    Point p2(1.0f, 2.0f, 3.0f);
    Vec v1(4.0f, 5.0f, 6.0f);
    Vec v2(2.0f, 1.0f, 3.0f);

    SUBCASE("Point -= Vec -> Point") {
        Point p_mut = p1;
        p_mut -= v1;
        CHECK(p_mut.is_close(Point(6.0f, 15.0f, 24.0f)));
    }

    SUBCASE("Point - Vec -> Point") {
        Point res = p1 - v1;
        CHECK(res.is_close(Point(6.0f, 15.0f, 24.0f)));
    }

    SUBCASE("Vec -= Vec -> Vec") {
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
// TEST 3: Negation (Unary Operator -)
// =============================================================================
TEST_CASE("Test 3: Negation Operations") {
    Point p(1.0f, -2.0f, 3.0f);
    Vec v(-4.0f, 5.0f, -6.0f);
    Normal n(1.0f, 0.0f, 0.0f);

    SUBCASE("Negate Vec") {
        Vec res = -v;
        CHECK(res.is_close(Vec(4.0f, -5.0f, 6.0f)));
    }

    SUBCASE("Negate Point") {
        Point res = -p;
        CHECK(res.is_close(Point(-1.0f, 2.0f, -3.0f)));
    }

    SUBCASE("Negate Normal") {
        Normal res = -n;
        CHECK(res.is_close(Normal(-1.0f, 0.0f, 0.0f)));
    }
}

// =============================================================================
// TEST 4: Scalar Multiplications
// =============================================================================
TEST_CASE("Test 4: Scalar Multiplications") {
    Point p(1.0f, 2.0f, 3.0f);
    Vec v(4.0f, 5.0f, 6.0f);

    float normalization = std::sqrt(1.0f + 4.0f + 9.0f);
    float xn = 1.0f/normalization;
    float yn = 2.0f/normalization;
    float zn = 3.0f/normalization;
    Normal n(xn, yn, zn);

    float scalar = 2.0f;

    Vec exp_sn(scalar*xn, scalar*yn, scalar*zn);

// These have been commented in Geometry.cppm
//    SUBCASE("Point *= scalar") {
//        Point p_mut = p;
//        p_mut *= scalar;
//        CHECK(aux::are_xyz_close(p_mut, Point(2.0f, 4.0f, 6.0f)));
//    }
//
//    SUBCASE("Point * scalar") {
//        Point res = p * scalar;
//        CHECK(aux::are_xyz_close(res, Point(2.0f, 4.0f, 6.0f)));
//    }
//
//    SUBCASE("scalar * Point") {
//        Point res = scalar * p;
//        CHECK(aux::are_xyz_close(res, Point(2.0f, 4.0f, 6.0f)));
//    }

    SUBCASE("Vec *= scalar -> Vec") {
        Vec v_mut = v;
        v_mut *= scalar;
        CHECK(v_mut.is_close(Vec(8.0f, 10.0f, 12.0f)));
    }

    SUBCASE("Vec * scalar -> Vec") {
        Vec res = v * scalar;
        CHECK(res.is_close(Vec(8.0f, 10.0f, 12.0f)));
    }

    SUBCASE("scalar * Vec -> Vec") {
        Vec res = scalar * v;
        CHECK(res.is_close(Vec(8.0f, 10.0f, 12.0f)));
    }

    SUBCASE("Normal * scalar -> Vec") {
        Vec res = n * scalar;
        CHECK(res.is_close(exp_sn));
    }

    SUBCASE("scalar * Normal -> Vec") {
        Vec res = scalar * n;
        CHECK(res.is_close(exp_sn));
    }
}

// =============================================================================
// TEST 5: Scalar division
// =============================================================================

TEST_CASE("Test 5: Scalar division") {
    Point p(1.0f, 2.0f, 3.0f);
    Vec v(4.0f, 5.0f, 6.0f);

    float normalization = std::sqrt(1.0f + 4.0f + 9.0f);
    float xn = 1.0f/normalization;
    float yn = 2.0f/normalization;
    float zn = 3.0f/normalization;
    Normal n(xn, yn, zn);

    float scalar = 2.0f;

    Vec exp_sn(xn/scalar, yn/scalar, zn/scalar);

    SUBCASE("Vec /= scalar -> Vec") {
        Vec v_mut = v;
        v_mut /= scalar;
        CHECK(v_mut.is_close(Vec(2.0f, 2.5f, 3.0f)));
    }

    SUBCASE("Vec / scalar -> Vec") {
        Vec res = v / scalar;
        CHECK(res.is_close(Vec(2.0f, 2.5f, 3.0f)));
    }

    SUBCASE("Normal / scalar -> Vec") {
        Vec res = n / scalar;
        CHECK(res.is_close(exp_sn));
    }

    SUBCASE("Point / scalar -> Vec") {
        Point res = p / scalar;
        CHECK(res.is_close(Point(0.5f, 1.0f, 1.5f)));
    }
}

// =============================================================================
// TEST 6: Dot Products
// =============================================================================
TEST_CASE("Test 6: Dot Products") {
    Vec v1(4.0f, -5.0f, 6.0f);
    Vec v2(-1.0f, 2.0f, -3.0f);
    Normal n1(0.0f, 1.0f, 0.0f);

    float normalization = std::sqrt(1.0f + 4.0f + 9.0f);
    Normal n2(1.0f/normalization, 2.0f/normalization, 3.0f/normalization);

    float exp_vn = (1.0f/normalization)*(4.0f) + (2.0f/normalization)*(-5.0f) + (3.0f/normalization)*(6.0f);
    float exp_nn =  2.0f/normalization;

    SUBCASE("Vec * Vec") {
        float res = v1 * v2; // (4 * -1) + (-5 * 2) + (6 * -3) = -4 - 10 - 18 = -32
        CHECK(aux::are_close(res, -32.0f));
    }

    SUBCASE("Vec * Normal") {
        float res1 = v1 * n1; // (4 * 0) + (-5 * 1) + (6 * 0) = -5
        CHECK(aux::are_close(res1, -5.0f));

        float res2 = v1 * n2;
        CHECK(aux::are_close(res2, exp_vn));
    }

    SUBCASE("Normal * Vec") {
        float res1 = n1 * v1; // (4 * 0) + (-5 * 1) + (6 * 0) = -5
        CHECK(aux::are_close(res1, -5.0f));

        float res2 = n2 * v1;
        CHECK(aux::are_close(res2, exp_vn));
    }

    SUBCASE("Normal * Normal") {
        float res = n1 * n2;
        CHECK(aux::are_close(res, exp_nn));
    }
}

// =============================================================================
// TEST 7: Cross products
// =============================================================================

TEST_CASE("Test 7: Cross Products") {
    Vec v1(4.0f, -5.0f, 6.0f);
    Vec v2(-1.0f, 2.0f, -3.0f);
    Normal n1(0.0f, 1.0f, 0.0f);

    float normalization = std::sqrt(1.0f + 4.0f + 9.0f);
    float xn = 1.0f/normalization;
    float yn = 2.0f/normalization;
    float zn = 3.0f/normalization;
    Normal n2(xn, yn, zn);

    Vec exp_vv(3.0f, 6.0f, 3.0f);
    Vec exp_vn1(-6.0f, 0.0f, 4.0f);
    Vec exp_vn2(-5.0f*zn-6.0f*yn, 6.0f*xn-4.0f*zn, 4.0f*yn+5.0f*xn);
    Vec exp_nn(1.0f*zn, 0.0f, -1.0f*xn);

    SUBCASE("Vec % Vec -> Vec") {
        Vec res = v1 % v2;
        CHECK(res.is_close(exp_vv));
    }

    SUBCASE("Vec % Normal -> Vec") {
        Vec res1 = v1 % n1;
        CHECK(res1.is_close(exp_vn1));

        Vec res2 = v1 % n2;
        CHECK(res2.is_close(exp_vn2));
    }

    SUBCASE("Normal % Vec -> Vec") {
        Vec res1 = n1 % v1;
        CHECK(res1.is_close(-exp_vn1));

        Vec res2 = n2 % v1;
        CHECK(res2.is_close(-exp_vn2));
    }

    SUBCASE("Normal * Normal") {
        Vec res = n1 % n2;
        CHECK(res.is_close(exp_nn));
    }
}

// =============================================================================
// TEST 8: Matrix multiplication
// =============================================================================

TEST_CASE("Test 8: Matricial operations") {
    HomMatrix M1{{1.0f, 2.0f, 3.0f, 4.0f,
                 5.0f, 6.0f, 7.0f, 8.0f,
                 9.0f, 10.0f, 11.0f, 12.0f,
                 0.0f, 0.0f, 0.0f, 1.0f}};

    HomMatrix M2{{10.0f, 20.0f, 30.0f, 40.0f,
                 50.0f, 60.0f, 70.0f, 80.0f,
                 90.0f, 100.0f, 110.0f, 120.0f,
                 00.0f, 00.0f, 00.0f, 1.0f}};

    Point p(1.0f, 2.0f, 3.0f);
    Vec v(4.0f, 5.0f, 6.0f);

    Point exp_Mp(18.0f, 46.0f, 74.0f);
    Vec exp_Mv(32.0f, 92.0f, 152.0f);
    HomMatrix exp_M{{
        380.0f,  440.0f,  500.0f,  564.0f,
        980.0f,  1160.0f, 1340.0f, 1528.0f,
        1580.0f, 1880.0f, 2180.0f, 2492.0f,
        0.0f,    0.0f,    0.0f,    1.0f
    }};

    SUBCASE("Matrix * Point -> Point") {
        Point res = M1 * p;
        CHECK(res.is_close(exp_Mp));
    }

    SUBCASE("Matrix * Vec -> Vec") {
        Vec res = M1 * v;
        CHECK(res.is_close(exp_Mv));
    }

    SUBCASE("Matrix * matrix -> Matrix") {
        HomMatrix res = M1 * M2;
        CHECK(res.is_close(exp_M));
    }

}

// =============================================================================
// TEST 9: Transformations
// =============================================================================

TEST_CASE("Test 9: Transformations") {

    SUBCASE("Base Operator Overloads (Transformation * Object)") {
        // Test only operators.
        Transformation T;

        // Direct matrix: scales for vector (2, 3, 4) and translates +10, +20, +30
        T.m = HomMatrix{{2.0f, 0.0f, 0.0f, 10.0f,
                         0.0f, 3.0f, 0.0f, 20.0f,
                         0.0f, 0.0f, 4.0f, 30.0f,
                         0.0f, 0.0f, 0.0f, 1.0f}};

        // Inverse matrix
        T.invm = HomMatrix{{1.0f/2.0f, 0.0f, 0.0f, -5.0f,
                              0.0f, 1.0f/3.0f, 0.0f, -6.666666f,
                              0.0f, 0.0f, 1.0f/4.0f, -7.5f,
                              0.0f, 0.0f, 0.0f, 1.0f}};

        Point p_test(1.0f, 2.0f, 3.0f);
        Vec v_test(1.0f, 2.0f, 3.0f);
        Normal n_test(1.0f, 2.0f, 4.0f);

        // 1. Transformation * Point
        // x = 2*1 + 10 = 12
        // y = 3*2 + 20 = 26
        // z = 4*3 + 30 = 42
        Point pt = T * p_test;
        CHECK(pt.is_close(Point(12.0f, 26.0f, 42.0f)));

        // 2. Transformation * Vec
        // x = 2*1 = 2
        // y = 3*2 = 6
        // z = 4*3 = 12
        Vec vt = T * v_test;
        CHECK(vt.is_close(Vec(2.0f, 6.0f, 12.0f)));

        // 3. Transformation * Normal
        // x = 0.5 * 1 = 0.5
        // y = 0.333333 * 2 = 0.666666
        // z = 0.25 * 4 = 1.0
        Normal nt = T * n_test;
        CHECK(nt.is_close(Normal(0.5f, 0.666666f, 1.0f), 1e-4f));
    }

    Point p(1.0f, 2.0f, 3.0f);
    Vec v(1.0f, 2.0f, 3.0f);
    Normal n(0.0f, 0.0f, 1.0f);

    SUBCASE("Translation") {
        Transformation tr = Trans(Vec(2.0f, 3.0f, 4.0f));

        CHECK(tr.is_consistent() == true);

        // Point is translated
        Point p_trans = tr * p;
        CHECK(p_trans.is_close(Point(3.0f, 5.0f, 7.0f)));

        // Vec is NOT translated
        Vec v_trans = tr * v;
        CHECK(v_trans.is_close(v));
    }

    SUBCASE("Scaling") {
        Transformation sc = Scale(Vec(2.0f, 3.0f, 4.0f));

        CHECK(sc.is_consistent() == true);

        Point p_scaled = sc * p;
        CHECK(p_scaled.is_close(Point(2.0f, 6.0f, 12.0f)));

        Vec v_scaled = sc * v;
        CHECK(v_scaled.is_close(Vec(2.0f, 6.0f, 12.0f)));

        // Normal transforms with inverse transpose!
        // If we scale Z by 4, the Normal Z should scale by 1/4 = 0.25
        Normal n_scaled = sc * n;
        CHECK(n_scaled.is_close(Normal(0.0f, 0.0f, 0.25f)));
    }

    SUBCASE("Rotations") {
        float pi_half = std::acos(-1.0f) / 2.0f; // 90 degrees
        Transformation rx = R_x(pi_half);
        Transformation ry = R_y(pi_half);
        Transformation rz = R_z(pi_half);

        CHECK(rx.is_consistent() == true);
        CHECK(ry.is_consistent() == true);
        CHECK(rz.is_consistent() == true);

        // Rotate Y axis around X by 90 deg -> becomes Z axis
        Vec vy(0.0f, 1.0f, 0.0f);
        CHECK((rx * vy).is_close(Vec(0.0f, 0.0f, 1.0f)));

        // Rotate X axis around Y by 90 deg -> becomes -Z axis
        Vec vx(1.0f, 0.0f, 0.0f);
        CHECK((ry * vx).is_close(Vec(0.0f, 0.0f, -1.0f)));
    }

    SUBCASE("Transformation Composition and Inverse") {
        Transformation tr = Trans(Vec(5.0f, 0.0f, 0.0f));
        Transformation sc = Scale(Vec(2.0f, 2.0f, 2.0f));

        Transformation combined = tr * sc; // Scale THEN translate
        CHECK(combined.is_consistent() == true);

        // Apply to point
        Point p_comb = combined * Point(1.0f, 1.0f, 1.0f); // Scale to 2,2,2 -> translate X by 5 -> 7,2,2
        CHECK(p_comb.is_close(Point(7.0f, 2.0f, 2.0f)));

        // Apply inverse
        Point p_orig = combined.inverse() * p_comb;
        CHECK(p_orig.is_close(Point(1.0f, 1.0f, 1.0f)));
    }
}

// =============================================================================
// TEST 10: Squared Length and Length (norm2, norm)
// =============================================================================
TEST_CASE("Test 8: Length and Squared Length") {
    SUBCASE("Vec::Normal and Vec::Normal2") {
        Vec v(3.0f, 4.0f, 5.0f); // Length should be 5

        CHECK(aux::are_close(v.norm(), 7.07106781f));
        CHECK(aux::are_close(v.norm2(), 50.0f));
    }

    SUBCASE("Normal::Normal and Normal::Normal2") {
        Normal n(0.6f, 0.8f, 0.0f); // Length should be exactly 1

        CHECK(aux::are_close(n.norm(), 1.0f));
        CHECK(aux::are_close(n.norm2(), 1.0f));
    }
}

// =============================================================================
// Test 11: Object conversion and normalization
// =============================================================================
TEST_CASE("Test 10: Object Conversions and Normalization (to_vec, to_Normal, Normalize)") {

    SUBCASE("Vec::Normalize") {
        // A vector of length 5
        Vec v(3.0f, 4.0f, 0.0f);
        Vec Normalized_v = v.normalize();

        // Must return a Vec (not a Normal) with length 1
        CHECK(Normalized_v.is_close(Vec(0.6f, 0.8f, 0.0f)));
    }

    SUBCASE("Normal::Normalize") {
        Normal n (3.0f, 4.0f, 0.0f);
        Normal Normalized_n = n.normalize();

        // Must return a Vec (not a Normal) with length 1
        CHECK(Normalized_n.is_close(Normal(0.6f, 0.8f, 0.0f)));
    }

    SUBCASE("Point::to_vec") {
        Point p(1.0f, 2.0f, 3.0f);
        Vec v = p.to_vec();
        CHECK(v.is_close(Vec(1.0f, 2.0f, 3.0f)));
    }

    SUBCASE("Vec::to_norm") {
        // A vector of length 5 (3-4-5 triangle on xy plane)
        Vec v(3.0f, 4.0f, 0.0f);
        Normal n = v.to_norm();

        // Expected Normalized values: x=3/5=0.6, y=4/5=0.8, z=0
        CHECK(n.is_close(Normal(0.6f, 0.8f, 0.0f)));
    }

}



// =============================================================================
// TEST 12: String Conversions
// =============================================================================
TEST_CASE("Test 12: String conversions") {
    Point p(1.3f, 4.6f, -7.8f);
    Vec v(4.6f, -33.9f, 0.012f);
    Normal n(1.0f, 0.0f, 0.0f);
    HomMatrix M{{1.0f, 2.0f, 3.0f, 4.0f,
                 5.0f, 6.0f, 7.0f, 8.0f,
                 9.0f, 10.0f, 11.0f, 12.0f,
                 0.0f, 0.0f, 0.0f, 1.0f}};

    std::string expected_matrix =
            "1.0 2.0 3.0 4.0\n"
            "5.0 6.0 7.0 8.0\n"
            "9.0 10.0 11.0 12.0\n"
            "0.0 0.0 0.0 1.0\n";

    CHECK(std::format("The Point components are Point({:.2f})", p) == "The Point components are Point(1.30 4.60 -7.80)");
    CHECK(std::format("The Vec components are Vec({:.3f})", v) == "The Vec components are Vec(4.600 -33.900 0.012)");
    CHECK(std::format("The Normal components are Normal({:.2f})", n) == "The Normal components are Normal(1.00 0.00 0.00)");
    CHECK(std::format("The HomMatrix components are HomMatrix(\n{:.1f})", M) == "The HomMatrix components are HomMatrix(\n" + expected_matrix + ")");
}