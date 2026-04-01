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

export module auxiliary_functions;
import std;


export namespace aux {

    // =========================================================
    // Checks if two numerical values are close
    // =========================================================

    bool are_close(float x, float y, float tolerance=1e-5f) {
        return std::abs(x - y) <= tolerance;
        
    }

    // ===================================================================
    // Checks if two objects with three coordinates x, y and z are close
    // ===================================================================
    template<typename T>
    bool are_xyz_close(const T& a, const T& b, float epsilon = 1e-5f) {
        return aux::are_close(a.x, b.x, epsilon) &&
               aux::are_close(a.y, b.y, epsilon) &&
               aux::are_close(a.z, b.z, epsilon);
    }

    // =========================================================
    // Managing of input/output file opening
    // =========================================================

    // No need to worry about closing streams: ifstream and ofstream close the stream automatically
    // just before they get out of scope wherever they are

    // Opens an input file and checks for anomalies
    std::expected<std::ifstream, std::string> open_input_file(const std::string& filename) {
        // Opens the input file in binary mode: the data in the file will be read as raw bytes
        std::ifstream stream(filename, std::ios::binary);

        if (!stream.is_open()) {
            return std::unexpected(std::format("Error in opening input file '{}'", filename));
        }

        return stream;
    }

    // Opens an output file and checks for anomalies
    std::expected<std::ofstream, std::string> open_output_file(const std::string& filename) {
        // Opens the input file in binary mode: the data in the file will be read as raw bytes
        std::ofstream stream(filename, std::ios::binary);
        if (!stream.is_open()) {
            return std::unexpected(std::format("Error in opening output file '{}'", filename));
        }

        return stream;
    }

}