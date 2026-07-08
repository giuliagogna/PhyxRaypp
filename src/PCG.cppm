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
 * @file PCG.cppm
 * @brief Permuted Congruential Generator (PCG) random number generator.
 *
 * This module implements a PCG-based pseudo-random number generator
 * used throughout the renderer for sampling and Monte Carlo integration.
 *
 * The implementation is based on:
 *
 * Melissa E. O'Neill,
 * "PCG: A Family of Simple Fast Space-Efficient Statistically Good
 * Algorithms for Random Number Generation", 2014.
 *
 * Reference:
 * https://www.pcg-random.org/
 */

module;

export module PCG;
import std;

/**
 * @brief PCG-based pseudo-random number generator.
 *
 * Generates uniformly distributed 32-bit integers and floating-point
 * values in the interval [0,1).
 */
export struct PCG {

    /// Internal generator state.
    std::uint64_t state;

    /// Stream selector used to generate independent sequences.
    std::uint64_t inc;

    /**
     * @brief Construct a generator with the given seed and sequence.
     *
     * Different sequence values produce independent random streams.
     *
     * @param init_state: Initial seed value.
     * @param init_seq: Sequence identifier.
     */
    PCG(std::uint64_t init_state = 42, std::uint64_t init_seq = 54) {
        state = 0;
        inc = (init_seq << 1) | 1u;
        random();
        state += init_state;
        random();
    }

    /**
     * @brief Generate a uniformly distributed 32-bit integer.
     *
     * @return Random unsigned integer.
     */
    std::uint32_t random() {
        std::uint64_t oldstate = state;
        state = oldstate * 6364136223846793005ULL + inc;

        // Apply the scrambling function (XorShift + RandomRotation)
        std::uint32_t xorshifted = static_cast<std::uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
        std::uint32_t rot = static_cast<std::uint32_t>(oldstate >> 59u);

        // Using the C++20/23 rotr() rotation function
        return std::rotr(xorshifted, rot);
    }

    /**
     * @brief Generate a uniformly distributed float in [0,1).
     *
     * @return Random floating-point value.
     */
    float random_float() {
        return static_cast<float>(random()) / 4294967296.0f;
    }
};