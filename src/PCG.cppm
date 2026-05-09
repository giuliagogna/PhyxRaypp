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

export module PCG;
import std;

export struct PCG {
    std::uint64_t state;
    std::uint64_t inc;

    PCG(std::uint64_t init_state = 42, std::uint64_t init_seq = 54) {
        state = 0;
        inc = (init_seq << 1) | 1u;
        random();
        state += init_state;
        random();
    }

    // It returns a random 32 bit integer
    std::uint32_t random() {
        std::uint64_t oldstate = state;
        state = oldstate * 6364136223846793005ULL + inc;

        // Apply the scrambling function (XorShift + RandomRotation)
        std::uint32_t xorshifted = static_cast<std::uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
        std::uint32_t rot = static_cast<std::uint32_t>(oldstate >> 59u);

        // Using the C++20/23 rotr rotation function
        return std::rotr(xorshifted, rot);
    }

    // It returns a uniform RGN in [0, 1)
    float random_float() {
        return static_cast<float>(random()) / 4294967296.0f;
    }
};