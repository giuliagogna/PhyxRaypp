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
import PCG;

// ==========================================================================
// ==========================================================================
// TESTING PCG RANDOM NUMBER GENERATOR
// ==========================================================================
// ==========================================================================

TEST_CASE("Test 1: expecting a correct sequence") {
    PCG pcg; // default (42, 54)

    CHECK(pcg.state == 1753877967969059832ULL);
    CHECK(pcg.inc == 109ULL);

    CHECK(pcg.random() == 2707161783u);
    CHECK(pcg.random() == 2068313097u);
    CHECK(pcg.random() == 3122475824u);
    CHECK(pcg.random() == 2211639955u);
    CHECK(pcg.random() == 3215226955u);
    CHECK(pcg.random() == 3421331566u);
}