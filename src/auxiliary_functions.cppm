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
 * @file auxiliary_functions.cppm
 * @brief Collection of utility functions used throughout the renderer.
 *
 * This module provides helpers for numerical comparisons and
 * file stream management.
 */

module;

export module auxiliary_functions;
import std;

/**
 * @brief Auxiliary utility functions.
 */
export namespace aux {

    /**
     * @brief Compare two floating-point values within a tolerance.
     *
     * @param x First value.
     * @param y Second value.
     * @param tolerance Maximum allowed difference.
     *
     * @return True if the values are sufficiently close.
     */
    bool are_close(float x, float y, float tolerance=1e-5f) {
        return std::abs(x - y) <= tolerance;
        
    }

    // ===================================================================
    // Checks if two objects with three coordinates x, y and z are close
    // ===================================================================

    /**
     * @brief Compare two 3D objects component-wise.
     *
     * The type T must expose x, y and z members.
     *
     * @param a First object.
     * @param b Second object.
     * @param epsilon Maximum allowed difference per component.
     *
     * @return True if all corresponding components are sufficiently close.
     */
    template<typename T>
    bool are_xyz_close(const T& a, const T& b, float epsilon = 1e-5f) {
        return aux::are_close(a.x, b.x, epsilon) &&
               aux::are_close(a.y, b.y, epsilon) &&
               aux::are_close(a.z, b.z, epsilon);
    }



    // =========================================================
    // File stream helpers
    // =========================================================

    /**
     * @brief Open an input file in binary mode.
     *
     * Returns an error message if the file cannot be opened.
     *
     * @param filename Path to the file.
     * @return Input stream or error description.
     */
    std::expected<std::ifstream, std::string> open_input_file(const std::string& filename) {
        // Read file as raw binary data.
        std::ifstream stream(filename, std::ios::binary);

        if (!stream.is_open()) {
            return std::unexpected(std::format("Error in opening file '{}'", filename));
        }

        return stream;
    }

    /**
     * @brief Open an output file in binary mode.
     *
     * Returns an error message if the file cannot be created.
     *
     * @param filename Path to the file.
     * @return Output stream or error description.
     */
    std::expected<std::ofstream, std::string> open_output_file(const std::string& filename) {
        // Opens the input file in binary mode: the data in the file will be read as raw bytes
        std::ofstream stream(filename, std::ios::binary);
        if (!stream.is_open()) {
            return std::unexpected(std::format("Error in opening output file '{}'", filename));
        }

        return stream;
    }

}